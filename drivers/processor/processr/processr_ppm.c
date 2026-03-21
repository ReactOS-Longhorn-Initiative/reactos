/*
 * PROJECT:     ReactOS Generic CPU Driver — Intel PPM (P-state) registration
 * LICENSE:     GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:     Register PROCESSOR_PERF_STATES via ZwPowerInformation.
 *              Prefers ACPI _PSS/_PCT when present (Windows intelppm-style);
 *              falls back to IA32_PLATFORM_INFO / IA32_PERF_CTL MSRs.
 */

#include "processr.h"

#include <intrin.h>
#include <ntpoapi.h>
#include <ioaccess.h>

#if (NTDDI_VERSION < NTDDI_WIN7)
NTKERNELAPI ULONG NTAPI KeGetCurrentProcessorIndex(VOID);
#endif

#define NDEBUG
#include <debug.h>

#define TAG_PROCESSR_PPM  'mrPP'

#define MSR_IA32_PLATFORM_INFO  0x000000CE
#define MSR_IA32_PERF_STATUS    0x00000198
#define MSR_IA32_PERF_CTL       0x00000199

#define PROCESSR_MAX_CPUS       256
#define PROCESSR_INTEL_PSTATES  16

#define ACPI_ADR_SPACE_SYSTEM_MEMORY  0
#define ACPI_ADR_SPACE_SYSTEM_IO      1
#define ACPI_ADR_SPACE_FIXED_HW       0x7F

static PROCESSR_PPM_DISPATCH_TABLE ProcessrPpmDispatch;
static BOOLEAN ProcessrPpmDispatchValid;

static PDEVICE_EXTENSION ProcessrPerfDevExtByCpu[PROCESSR_MAX_CPUS];

static __inline ULONG
ProcessrMinUlong(
    _In_ ULONG A,
    _In_ ULONG B)
{
    return (A < B) ? A : B;
}

static __inline ULONG
ProcessrMaximumLogicalProcessorCount(VOID)
{
#if (NTDDI_VERSION >= NTDDI_VISTA) && defined(SINGLE_GROUP_LEGACY_API)
    return KeQueryMaximumProcessorCount();
#else
    return (ULONG)KeNumberProcessors;
#endif
}

static VOID
ProcessrProcessorNumberFromIndex(
    _In_ ULONG ProcessorIndex,
    _Out_ PPROCESSOR_NUMBER ProcNumber)
{
#if (NTDDI_VERSION >= NTDDI_WIN7)
    if (NT_SUCCESS(KeGetProcessorNumberFromIndex(ProcessorIndex, ProcNumber)))
        return;
#endif
    ProcNumber->Group = 0;
    ProcNumber->Number = (UCHAR)ProcessorIndex;
    ProcNumber->Reserved = 0;
}

static VOID
ProcessrBindCurrentThreadToProcessorIndex(
    _In_ ULONG ProcessorIndex,
    _Out_ PGROUP_AFFINITY PreviousAffinity)
{
#if (NTDDI_VERSION >= NTDDI_WIN7)
    PROCESSOR_NUMBER pn;
    GROUP_AFFINITY aff;

    ProcessrProcessorNumberFromIndex(ProcessorIndex, &pn);
    RtlZeroMemory(&aff, sizeof(aff));
    aff.Group = pn.Group;
    aff.Mask = (KAFFINITY)1 << pn.Number;
    KeSetSystemGroupAffinityThread(&aff, PreviousAffinity);
#else
    UNREFERENCED_PARAMETER(PreviousAffinity);
    KeSetSystemAffinityThread((KAFFINITY)((ULONG_PTR)1 << ProcessorIndex));
#endif
}

static VOID
ProcessrRestoreThreadAffinity(
    _In_ PGROUP_AFFINITY PreviousAffinity)
{
#if (NTDDI_VERSION >= NTDDI_WIN7)
    KeRevertToUserGroupAffinityThread(PreviousAffinity);
#else
    UNREFERENCED_PARAMETER(PreviousAffinity);
    KeRevertToUserAffinityThread();
#endif
}

static NTSTATUS
ProcessrQueryInstanceIndex(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PULONG ProcessorIndex)
{
    PIO_STACK_LOCATION IrpStack;
    IO_STATUS_BLOCK IoStatus;
    PDEVICE_OBJECT TargetObject;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    UNICODE_STRING Us;
    ULONG Index;

    PAGED_CODE();

    *ProcessorIndex = 0;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    TargetObject = IoGetAttachedDeviceReference(DeviceObject);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       TargetObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);
    if (Irp == NULL)
    {
        ObDereferenceObject(TargetObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IrpStack = IoGetNextIrpStackLocation(Irp);
    RtlZeroMemory(IrpStack, sizeof(IO_STACK_LOCATION));
    IrpStack->MajorFunction = IRP_MJ_PNP;
    IrpStack->MinorFunction = IRP_MN_QUERY_ID;
    IrpStack->Parameters.QueryId.IdType = BusQueryInstanceID;

    Status = IoCallDriver(TargetObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (!NT_SUCCESS(Status) || IoStatus.Information == 0)
    {
        ObDereferenceObject(TargetObject);
        return Status;
    }

    RtlInitUnicodeString(&Us, (PWSTR)IoStatus.Information);
    Status = RtlUnicodeStringToInteger(&Us, 10, &Index);
    ExFreePool((PVOID)IoStatus.Information);
    ObDereferenceObject(TargetObject);

    if (!NT_SUCCESS(Status))
        return Status;

    *ProcessorIndex = Index;
    return STATUS_SUCCESS;
}

static BOOLEAN
ProcessrIsIntelCpu(VOID)
{
    INT CpuInfo[4];

    __cpuid(CpuInfo, 0);
    if (CpuInfo[0] < 1)
        return FALSE;

    return (CpuInfo[1] == 0x756e6547) &&
           (CpuInfo[3] == 0x49656e69) &&
           (CpuInfo[2] == 0x6c65746e);
}

static VOID
ProcessrIntelProbeRatios(
    _Out_ PULONG MaxRatio,
    _Out_ PULONG MinRatio,
    _Out_ PULONG NominalMhz)
{
    INT CpuInfo[4];
    ULONG64 Plat;
    ULONG64 StatusMsr;
    ULONG maxR, minR;

    *MaxRatio = 0;
    *MinRatio = 8;
    *NominalMhz = 0;

    __cpuid(CpuInfo, 0);
    if ((ULONG)CpuInfo[0] >= 0x16)
    {
        __cpuid(CpuInfo, 0x16);
        if ((ULONG)CpuInfo[0] != 0)
            *NominalMhz = (ULONG)CpuInfo[0];
    }

    Plat = __readmsr(MSR_IA32_PLATFORM_INFO);
    maxR = (ULONG)((Plat >> 8) & 0xFF);
    minR = (ULONG)((Plat >> 16) & 0xFF);

    if (maxR == 0)
    {
        StatusMsr = __readmsr(MSR_IA32_PERF_STATUS);
        maxR = (ULONG)(StatusMsr & 0xFFFF);
    }

    if (maxR == 0)
        maxR = 20;

    if (minR == 0 || minR > maxR)
        minR = ProcessrMinUlong(8, maxR);

    if (minR >= maxR && maxR > 1)
        minR = maxR - 1;

    *MaxRatio = maxR;
    *MinRatio = minR;

    if (*NominalMhz == 0)
        *NominalMhz = maxR * 100;
}

static VOID
ProcessrPerfRegisterCpuTable(
    _In_ PDEVICE_EXTENSION DevExt)
{
    ULONG Cpu = KeGetCurrentProcessorIndex();

    if (Cpu < PROCESSR_MAX_CPUS)
        ProcessrPerfDevExtByCpu[Cpu] = DevExt;
}

static VOID
ProcessrPerfUnregisterCpuTable(
    _In_ PDEVICE_EXTENSION DevExt)
{
    ULONG Cpu = KeGetCurrentProcessorIndex();

    if (Cpu < PROCESSR_MAX_CPUS && ProcessrPerfDevExtByCpu[Cpu] == DevExt)
        ProcessrPerfDevExtByCpu[Cpu] = NULL;
}

static NTSTATUS
ProcessrMapPctSystemMemory(
    _Inout_ PDEVICE_EXTENSION DevExt)
{
    PPROCESSR_GEN_ADDR Ga;
    PHYSICAL_ADDRESS Pa;
    SIZE_T spanBytes;
    ULONG bitW;

    PAGED_CODE();

    if (!DevExt->AcpiPctValid)
        return STATUS_SUCCESS;

    Ga = &DevExt->AcpiPct.Control;
    if (Ga->AddressSpaceID != ACPI_ADR_SPACE_SYSTEM_MEMORY)
        return STATUS_SUCCESS;

    if (DevExt->AcpiPctMemoryVirt)
        return STATUS_SUCCESS;

    bitW = Ga->RegisterBitWidth ? Ga->RegisterBitWidth : 32;
    spanBytes = (SIZE_T)((Ga->RegisterBitOffset + bitW + 7) / 8);
    if (spanBytes < sizeof(ULONG))
        spanBytes = sizeof(ULONG);
    if (spanBytes > 0x1000)
        return STATUS_INVALID_PARAMETER;

    Pa = Ga->Address;
    DevExt->AcpiPctMemoryVirt = MmMapIoSpace(Pa, spanBytes, MmNonCached);
    if (!DevExt->AcpiPctMemoryVirt)
        return STATUS_INSUFFICIENT_RESOURCES;

    DevExt->AcpiPctMemoryBytes = spanBytes;
    return STATUS_SUCCESS;
}

static VOID
ProcessrWriteMemoryFromGenAddr(
    _In_ PDEVICE_EXTENSION DevExt,
    _In_ ULONG Value)
{
    PPROCESSR_GEN_ADDR Ga = &DevExt->AcpiPct.Control;
    PUCHAR base = DevExt->AcpiPctMemoryVirt;
    ULONG bitOff = Ga->RegisterBitOffset;
    ULONG bitW = Ga->RegisterBitWidth ? Ga->RegisterBitWidth : 32;
    ULONG64 mask;
    ULONG64 cur;
    ULONG64 fieldMask;

    if (!base)
        return;

    if (bitOff == 0 && bitW == 32)
    {
        WRITE_REGISTER_ULONG((PULONG)base, Value);
        return;
    }

    if (bitOff == 0 && bitW == 16)
    {
        WRITE_REGISTER_USHORT((PUSHORT)base, (USHORT)Value);
        return;
    }

    if (bitOff == 0 && bitW == 8)
    {
        WRITE_REGISTER_UCHAR(base, (UCHAR)Value);
        return;
    }

    if (bitOff + bitW > 32)
        return;

    cur = READ_REGISTER_ULONG((PULONG)base);
    fieldMask = (1ULL << bitW) - 1ULL;
    mask = fieldMask << bitOff;
    cur = (cur & ~mask) | (((ULONG64)Value & fieldMask) << bitOff);
    WRITE_REGISTER_ULONG((PULONG)base, (ULONG)cur);
}

static VOID
ProcessrWriteIoPortFromGenAddr(
    _In_ PPROCESSR_GEN_ADDR Ga,
    _In_ ULONG Value)
{
    PVOID Port = (PVOID)(ULONG_PTR)(USHORT)Ga->Address.LowPart;

    switch (Ga->AccessSize)
    {
    case 1:
        WRITE_PORT_UCHAR(Port, (UCHAR)Value);
        break;
    case 2:
        WRITE_PORT_USHORT(Port, (USHORT)Value);
        break;
    default:
        WRITE_PORT_ULONG(Port, Value);
        break;
    }
}

static VOID
ProcessrWriteMsrFromGenAddr(
    _In_ PPROCESSR_GEN_ADDR Ga,
    _In_ ULONG Value)
{
    ULONG msr = Ga->Address.LowPart;
    ULONG width = Ga->RegisterBitWidth ? Ga->RegisterBitWidth : 16;
    ULONG offset = Ga->RegisterBitOffset;
    ULONG64 fieldMask;
    ULONG64 mask;
    ULONG64 v;

    if (width + offset > 64)
        width = (offset < 64) ? (64 - offset) : 1;

    if (width == 0)
        width = 16;

    fieldMask = (1ULL << width) - 1ULL;
    mask = fieldMask << offset;

    v = __readmsr(msr);
    v = (v & ~mask) | (((ULONG64)Value & fieldMask) << offset);
    __writemsr(msr, v);
}

static VOID
ProcessrWritePerfControlValue(
    _In_ PDEVICE_EXTENSION DevExt,
    _In_ ULONG PssIndex)
{
    ULONG ctrl;
    PPROCESSR_PSS Pss = DevExt->AcpiPss;

    if (!Pss || PssIndex >= Pss->Count)
        return;

    ctrl = Pss->States[PssIndex].Control;

    if (DevExt->AcpiPctValid)
    {
        PPROCESSR_GEN_ADDR Ga = &DevExt->AcpiPct.Control;

        if (Ga->AddressSpaceID == ACPI_ADR_SPACE_SYSTEM_IO)
        {
            ProcessrWriteIoPortFromGenAddr(Ga, ctrl);
            return;
        }

        if (Ga->AddressSpaceID == ACPI_ADR_SPACE_SYSTEM_MEMORY &&
            DevExt->AcpiPctMemoryVirt)
        {
            ProcessrWriteMemoryFromGenAddr(DevExt, ctrl);
            return;
        }

        if (Ga->AddressSpaceID == ACPI_ADR_SPACE_FIXED_HW)
        {
            ProcessrWriteMsrFromGenAddr(Ga, ctrl);
            return;
        }
    }

    {
        ULONG64 ctl = __readmsr(MSR_IA32_PERF_CTL);
        ctl = (ctl & ~(ULONG64)0xFFFFULL) | (ctrl & 0xFFFFULL);
        __writemsr(MSR_IA32_PERF_CTL, ctl);
    }
}

static ULONG FASTCALL
ProcessrIntelPerfSelection(
    _In_ ULONG Context,
    _In_ ULONG TargetPercent,
    _In_ ULONG MinPercent,
    _In_ ULONG MaxPercent,
    _In_ ULONG Flags,
    _Out_ PULONG Frequency,
    _Out_ PULONGLONG Selection)
{
    PDEVICE_EXTENSION DevExt;
    ULONG Cpu;
    ULONG slowest, idx, t, pct;
    ULONG nom, maxR;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Flags);

    if (!Frequency || !Selection)
        return TargetPercent;

    Cpu = KeGetCurrentProcessorIndex();
    if (Cpu >= PROCESSR_MAX_CPUS)
        return TargetPercent;

    DevExt = ProcessrPerfDevExtByCpu[Cpu];
    if (!DevExt || DevExt->IntelPerfStateCount == 0)
    {
        *Frequency = 0;
        *Selection = 0;
        return TargetPercent;
    }

    if (DevExt->AcpiPss && DevExt->AcpiPss->Count > 0)
    {
        PPROCESSR_PSS Pss = DevExt->AcpiPss;

        nom = Pss->States[0].CoreFrequency;
        if (nom == 0)
        {
            *Frequency = 0;
            *Selection = 0;
            return TargetPercent;
        }

        slowest = Pss->Count - 1;

        t = TargetPercent;
        if (t < MinPercent)
            t = MinPercent;
        if (t > MaxPercent)
            t = MaxPercent;

        idx = ((100 - t) * slowest) / 100;
        if (idx > slowest)
            idx = slowest;

        *Frequency = Pss->States[idx].CoreFrequency;
        *Selection = idx;
        pct = (*Frequency * 100) / nom;
        if (pct > 100)
            pct = 100;
        return pct;
    }

    nom = DevExt->IntelNominalMhz;
    maxR = DevExt->IntelMaxRatio;
    if (nom == 0 || maxR == 0)
    {
        *Frequency = 0;
        *Selection = 0;
        return TargetPercent;
    }

    slowest = DevExt->IntelPerfStateCount - 1;

    t = TargetPercent;
    if (t < MinPercent)
        t = MinPercent;
    if (t > MaxPercent)
        t = MaxPercent;

    idx = ((100 - t) * slowest) / 100;
    if (idx > slowest)
        idx = slowest;

    *Selection = idx;
    *Frequency = (nom * (ULONG)DevExt->IntelPerfRatio[idx]) / maxR;
    if (*Frequency > nom)
        *Frequency = nom;

    pct = (*Frequency * 100) / nom;
    if (pct > 100)
        pct = 100;
    return pct;
}

static VOID FASTCALL
ProcessrIntelPerfControl(
    _In_ ULONG Context,
    _In_ ULONGLONG SelectedState,
    _In_ ULONG MinPercent,
    _In_ ULONG MaxPercent,
    _In_ ULONG TolerancePercent,
    _In_ UCHAR Autonomous,
    _In_ UCHAR Initiate,
    _In_ UCHAR Force)
{
    PDEVICE_EXTENSION DevExt;
    ULONG Cpu;
    ULONG idx;
    ULONG ratio;
    ULONG64 ctl;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(MinPercent);
    UNREFERENCED_PARAMETER(MaxPercent);
    UNREFERENCED_PARAMETER(TolerancePercent);
    UNREFERENCED_PARAMETER(Autonomous);

    if (!Initiate && !Force)
        return;

    Cpu = KeGetCurrentProcessorIndex();
    if (Cpu >= PROCESSR_MAX_CPUS)
        return;

    DevExt = ProcessrPerfDevExtByCpu[Cpu];
    if (!DevExt || DevExt->IntelPerfStateCount == 0)
        return;

    idx = (ULONG)SelectedState;
    if (idx >= (ULONG)DevExt->IntelPerfStateCount)
        idx = DevExt->IntelPerfStateCount - 1;

    if (DevExt->AcpiPss && DevExt->AcpiPss->Count > 0)
    {
        ProcessrWritePerfControlValue(DevExt, idx);
        return;
    }

    ratio = DevExt->IntelPerfRatio[idx];

    ctl = __readmsr(MSR_IA32_PERF_CTL);
    ctl = (ctl & ~(ULONG64)0xFFFFULL) | (ratio & 0xFFFFULL);
    __writemsr(MSR_IA32_PERF_CTL, ctl);
}

NTSTATUS
NTAPI
ProcessrInitPpmDispatch(VOID)
{
    NTSTATUS Status;

    RtlZeroMemory(&ProcessrPpmDispatch, sizeof(ProcessrPpmDispatch));

    Status = ZwPowerInformation(ProcessorStateHandler,
                                NULL,
                                0,
                                &ProcessrPpmDispatch,
                                sizeof(ProcessrPpmDispatch));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Processr: ZwPowerInformation(ProcessorStateHandler) failed 0x%08lx\n",
                Status);
        ProcessrPpmDispatchValid = FALSE;
        return Status;
    }

    if (ProcessrPpmDispatch.InterfaceVersion != PROCESSR_PPM_DRIVER_INTERFACE_VERSION)
    {
        DPRINT1("Processr: PPM interface version mismatch (got %lu, want %u)\n",
                ProcessrPpmDispatch.InterfaceVersion,
                PROCESSR_PPM_DRIVER_INTERFACE_VERSION);
        ProcessrPpmDispatchValid = FALSE;
        return STATUS_NOT_SUPPORTED;
    }

    ProcessrPpmDispatchValid = TRUE;
    return STATUS_SUCCESS;
}

VOID
ProcessrRegisterIntelPerfIfNeeded(
    _Inout_ PDEVICE_EXTENSION DevExt)
{
    NTSTATUS Status;
    ULONG ProcessorIndex;
    GROUP_AFFINITY PrevAffinity;
    PPROCESSOR_PERF_STATES Perf;
    PPROCESSOR_PERF_INFO ProcInfo;
    ULONG maxR, minR, nom;
    ULONG i;
    UCHAR nStates;

#if !defined(_M_IX86) && !defined(_M_AMD64)
    UNREFERENCED_PARAMETER(DevExt);
    return;
#else
    if (!ProcessrPpmDispatchValid ||
        !ProcessrPpmDispatch.RegisterPerfStates ||
        !ProcessrIsIntelCpu())
    {
        return;
    }

    if (DevExt->PpmRegistered)
        return;

    Status = ProcessrQueryInstanceIndex(DevExt->DeviceObject, &ProcessorIndex);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Processr: no numeric ACPI instance ID, skip PPM (0x%08lx)\n", Status);
        return;
    }

    if (ProcessorIndex >= ProcessrMaximumLogicalProcessorCount())
    {
        DPRINT1("Processr: instance CPU index %lu out of range\n", ProcessorIndex);
        return;
    }

    DevExt->ProcessorIndex = ProcessorIndex;

    if (DevExt->LowerDevice)
        (VOID)ProcessrCollectAcpiPerfStates(DevExt->LowerDevice, DevExt);

    ProcessrBindCurrentThreadToProcessorIndex(ProcessorIndex, &PrevAffinity);

    if (DevExt->AcpiPctValid &&
        DevExt->AcpiPct.Control.AddressSpaceID == ACPI_ADR_SPACE_SYSTEM_MEMORY)
    {
        Status = ProcessrMapPctSystemMemory(DevExt);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Processr: _PCT memory MmMapIoSpace failed 0x%08lx\n", Status);
        }
    }

    if (DevExt->AcpiPss && DevExt->AcpiPss->Count > 0)
    {
        PPROCESSR_PSS Pss = DevExt->AcpiPss;

        nom = Pss->States[0].CoreFrequency;
        if (nom == 0)
            nom = 1000;

        DevExt->IntelPerfStateCount = (UCHAR)Pss->Count;
        DevExt->IntelNominalMhz = nom;
        DevExt->IntelMaxRatio = 1;
    }
    else
    {
        ProcessrIntelProbeRatios(&maxR, &minR, &nom);

        nStates = PROCESSR_INTEL_PSTATES;
        for (i = 0; i < nStates; i++)
        {
            ULONG span = (maxR > minR) ? (maxR - minR) : 1;
            ULONG off = (span * i) / (nStates - 1);
            ULONG r = maxR - off;

            DevExt->IntelPerfRatio[i] = (UCHAR)ProcessrMinUlong(r, 0xFF);
        }

        DevExt->IntelPerfStateCount = nStates;
        DevExt->IntelNominalMhz = nom;
        DevExt->IntelMaxRatio = maxR;
    }

    Perf = ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(PROCESSOR_PERF_STATES),
                                 TAG_PROCESSR_PPM);
    if (!Perf)
    {
        ProcessrReleaseAcpiPerfData(DevExt);
        ProcessrRestoreThreadAffinity(&PrevAffinity);
        return;
    }

    ProcInfo = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(PROCESSOR_PERF_INFO),
                                     TAG_PROCESSR_PPM);
    if (!ProcInfo)
    {
        ExFreePoolWithTag(Perf, TAG_PROCESSR_PPM);
        ProcessrReleaseAcpiPerfData(DevExt);
        ProcessrRestoreThreadAffinity(&PrevAffinity);
        return;
    }

    RtlZeroMemory(Perf, sizeof(*Perf));
    RtlZeroMemory(ProcInfo, sizeof(*ProcInfo));

    ProcessrProcessorNumberFromIndex(ProcessorIndex, &ProcInfo->Number);

    ProcInfo->PerfContext = 0;
    ProcInfo->PlatformCap = 100;
    ProcInfo->ThermalCap = 100;
    ProcInfo->LimitReasons = 0;

    Perf->Version = PROCESSOR_PERF_STATES_VERSION;
    Perf->ProcessorCount = 1;
    Perf->Processors = ProcInfo;
    Perf->MaxPerfPercent = 100;
    Perf->MinPerfPercent = 0;
    Perf->MinThrottlePercent = 0;
    Perf->GlobalContext = 0;
    Perf->PerfSelectionHandler = ProcessrIntelPerfSelection;
    Perf->PerfControlHandler = ProcessrIntelPerfControl;

    if (DevExt->AcpiPss && DevExt->AcpiPss->Count > 0)
    {
        PPROCESSR_PSS Pss = DevExt->AcpiPss;
        ULONG last = Pss->Count - 1;

        Perf->NominalFrequency = Pss->States[0].CoreFrequency;
        if (Perf->NominalFrequency == 0)
            Perf->NominalFrequency = DevExt->IntelNominalMhz;

        Perf->NominalRelativePerformance =
            (ULONGLONG)Pss->States[0].CoreFrequency;
        Perf->MinimumRelativePerformance =
            (ULONGLONG)Pss->States[last].CoreFrequency;

        if (DevExt->AcpiPctValid &&
            DevExt->AcpiPct.Control.AddressSpaceID == ACPI_ADR_SPACE_SYSTEM_IO)
        {
            Perf->Type = PPM_PERF_STATE_TYPE_ACPI_IO;
        }
        else
        {
            Perf->Type = PPM_PERF_STATE_TYPE_ACPI_MSR;
        }
    }
    else
    {
        Perf->Type = PPM_PERF_STATE_TYPE_ACPI_MSR;
        Perf->NominalFrequency = nom;
        Perf->NominalRelativePerformance = (ULONGLONG)maxR * 100ULL;
        Perf->MinimumRelativePerformance = (ULONGLONG)minR * 100ULL;
    }

    ProcessrPerfRegisterCpuTable(DevExt);

    Status = ProcessrPpmDispatch.RegisterPerfStates(Perf);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Processr: RegisterPerfStates failed 0x%08lx\n", Status);
        ProcessrPerfUnregisterCpuTable(DevExt);
        ExFreePoolWithTag(ProcInfo, TAG_PROCESSR_PPM);
        ExFreePoolWithTag(Perf, TAG_PROCESSR_PPM);
        ProcessrReleaseAcpiPerfData(DevExt);
        ProcessrRestoreThreadAffinity(&PrevAffinity);
        return;
    }

    DevExt->KernelPerfStates = Perf;
    DevExt->KernelPerfProcInfo = ProcInfo;
    DevExt->PpmRegistered = TRUE;

    ProcessrRestoreThreadAffinity(&PrevAffinity);

    DPRINT("Processr: Intel PPM registered for CPU %lu (ACPI=%u nom=%lu MHz)\n",
           ProcessorIndex,
           DevExt->AcpiPss ? 1U : 0U,
           Perf->NominalFrequency);
#endif
}

VOID
ProcessrUnregisterIntelPerf(
    _Inout_ PDEVICE_EXTENSION DevExt)
{
    GROUP_AFFINITY PrevAffinity;

#if !defined(_M_IX86) && !defined(_M_AMD64)
    UNREFERENCED_PARAMETER(DevExt);
    return;
#else
    if (!DevExt->PpmRegistered)
        return;

    ProcessrBindCurrentThreadToProcessorIndex(DevExt->ProcessorIndex, &PrevAffinity);

    if (ProcessrPpmDispatchValid && ProcessrPpmDispatch.RegisterPerfStates)
        (VOID)ProcessrPpmDispatch.RegisterPerfStates(NULL);

    ProcessrPerfUnregisterCpuTable(DevExt);

    if (DevExt->KernelPerfProcInfo)
    {
        ExFreePoolWithTag(DevExt->KernelPerfProcInfo, TAG_PROCESSR_PPM);
        DevExt->KernelPerfProcInfo = NULL;
    }

    if (DevExt->KernelPerfStates)
    {
        ExFreePoolWithTag(DevExt->KernelPerfStates, TAG_PROCESSR_PPM);
        DevExt->KernelPerfStates = NULL;
    }

    DevExt->PpmRegistered = FALSE;

    ProcessrReleaseAcpiPerfData(DevExt);

    ProcessrRestoreThreadAffinity(&PrevAffinity);
#endif
}

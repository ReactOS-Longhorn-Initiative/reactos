/*
 * PROJECT:     ReactOS AMD Processor Power Management Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        drivers/processor/amdppm/amdppm.c
 * PURPOSE:     Driver entry point, global initialisation, and WDF callbacks.
 *
 *              The AMD PPM driver manages power management for AMD processors.
 *              One instance of the FDO (Functional Device Object) is created
 *              for each logical processor enumerated by the HAL/ACPI bus.
 *
 *              Startup sequence:
 *                1. DriverEntry       – create WDFDRIVER, query processor count
 *                2. ProcLibGlobalInit – retrieve PPM dispatch table from kernel
 *                3. EvtDriverDeviceAdd – create WDFDEVICE per CPU, register callbacks
 *                4. EvtDevicePrepareHardware – acquire ACPI interfaces, enum methods,
 *                                              register C/P/T-states with the kernel
 *
 * REFERENCES:  Windows 10 amdppm.sys (IDA decompilation analysis)
 *
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 */

/* INCLUDES ******************************************************************/

#include "amdppm.h"

#if defined(_M_IX86) || defined(_M_AMD64)
#include <intrin.h> /* _mm_monitor / _mm_mwait */
#endif

/* GLOBALS *******************************************************************/

/*
 * Single global instance of driver-wide state.
 * Initialised in ProcLibGlobalInit, protected by AmdPpmGlobals.Mutex.
 */
AMDPPM_GLOBALS AmdPpmGlobals;

/* IDLE-STATE IDLE CALLBACKS *************************************************
 *
 * Minimal C-state transition callbacks for the AMD MWAIT-based idle path.
 * These are passed to the kernel inside PROCESSOR_IDLE_STATES_EX so the
 * kernel idle dispatcher can enter C2/C3 on AMD CPUs.
 *
 * For now we use C1 halt (the processor's native WFI/HLT) as the only
 * idle state, which always works.  Deeper MWAIT-based C-states are added
 * once CST parsing is proven.
 */

static
VOID
FASTCALL
AmdIdlePrepare(
    _In_ PPROCESSOR_IDLE_PREPARE_INFO Info)
{
    UNREFERENCED_PARAMETER(Info);
    /* Nothing to do before entering C1-equivalent idle */
}

static
VOID
FASTCALL
AmdIdleCancel(
    _In_ PVOID Context,
    _In_ ULONG StateIndex)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(StateIndex);
}

static
ULONG
FASTCALL
AmdIdlePreselect(
    _In_ PVOID Context,
    _In_ PPROCESSOR_IDLE_CONSTRAINTS Constraints)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Constraints);
    return 0; /* Always select state 0 (C1) */
}

static
ULONG
FASTCALL
AmdIdleTest(
    _In_ PVOID Context,
    _In_ ULONG StateIndex,
    _In_ ULONG Duration)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(StateIndex);
    UNREFERENCED_PARAMETER(Duration);
    return 1; /* State is available */
}

static
ULONG
FASTCALL
AmdIdleAvailabilityCheck(
    _In_ PVOID Context,
    _In_ ULONG StateIndex)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(StateIndex);
    return 1;
}

static
NTSTATUS
FASTCALL
AmdIdlePreExecute(
    _In_  PVOID  Context,
    _In_  ULONG  StateIndex,
    _In_  ULONG  ProcessorIndex,
    _In_  ULONG  Flags,
    _Out_ PULONG Hint)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(StateIndex);
    UNREFERENCED_PARAMETER(ProcessorIndex);
    UNREFERENCED_PARAMETER(Flags);
    *Hint = 0;
    return STATUS_SUCCESS;
}

static
NTSTATUS
FASTCALL
AmdIdleExecute(
    _In_  PVOID  Context,
    _In_  ULONG  StateIndex,
    _In_  ULONG  ProcessorIndex,
    _In_  ULONG  Flags,
    _Out_ PULONG Hint)
{
    PFDO_DATA DevExt = (PFDO_DATA)Context;

    UNREFERENCED_PARAMETER(ProcessorIndex);
    UNREFERENCED_PARAMETER(Flags);

    *Hint = 0;

    /*
     * C1 / fallback: HLT.  Deeper ACPI indices use MONITOR/MWAIT when the CPU
     * advertises MONITOR/MWAIT (AMD_CAP_CST from cpuid.c).
     */
    if (StateIndex == 0 ||
        DevExt == NULL ||
        !(DevExt->DrvCapabilities & AMD_CAP_CST))
    {
        _enable();
        __halt();
        return STATUS_SUCCESS;
    }

#if defined(_M_IX86) || defined(_M_AMD64)
    {
        volatile ULONG_PTR MonitorWord = 0;
        ULONG Hints = (StateIndex & 0x0F) << 4;

        _mm_monitor((void const *)&MonitorWord, 0, 0);
        _enable();
        _mm_mwait(0, Hints);
    }
    return STATUS_SUCCESS;
#else
    _enable();
    __halt();
    return STATUS_SUCCESS;
#endif
}

static
VOID
FASTCALL
AmdIdleComplete(
    _In_ PVOID  Context,
    _In_ ULONG  StateIndex,
    _In_ ULONG  ProcessorIndex,
    _In_ ULONG  Flags,
    _In_ PULONG Hint)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(StateIndex);
    UNREFERENCED_PARAMETER(ProcessorIndex);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Hint);
}

static
BOOLEAN
FASTCALL
AmdIdleIsHalted(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
    return FALSE;
}

static
BOOLEAN
FASTCALL
AmdIdleInitiateWake(
    _In_ PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
    return TRUE;
}

/* REGISTRATION HELPERS ******************************************************/

/*
 * RegisterKernelIdleStates
 *
 * Builds a PROCESSOR_IDLE_STATES_EX from the ACPI _CST table stored in
 * DevExt->CST (or a single synthetic C1 state if CST is absent) and
 * registers it with the kernel power manager via the PPM dispatch table.
 *
 * The kernel stores this pointer in Prcb->PowerState.IdleState and uses
 * the callbacks to drive deep idle transitions.
 */
NTSTATUS
RegisterKernelIdleStates(
    _In_ PFDO_DATA DevExt)
{
    NTSTATUS Status;
    ULONG NumStates;
    ULONG AllocSize;
    PPROCESSOR_IDLE_STATES_EX IdleStates;
    ULONG i;

    PAGED_CODE();

    /*
     * Determine how many C-state entries to expose.
     * If _CST was not found or returned no states, fall back to a single
     * synthetic C1 entry so the kernel at least gets a valid idle handler.
     */
    NumStates = (DevExt->CST && DevExt->CST->Count > 0) ? DevExt->CST->Count : 1;

    AllocSize = FIELD_OFFSET(PROCESSOR_IDLE_STATES_EX, State) +
                NumStates * sizeof(PROCESSOR_IDLE_STATE_EX);

    IdleStates = ExAllocatePoolWithTag(NonPagedPool, AllocSize, TAG_AMDPPM_GENERIC);
    if (!IdleStates)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(IdleStates, AllocSize);

    /* Header */
    IdleStates->Version              = 1;
    IdleStates->Processor.Group      = 0;
    IdleStates->Processor.Number     = (UCHAR)KeGetCurrentProcessorNumber();
    IdleStates->Context              = DevExt;
    IdleStates->EstimateIdleDuration = FALSE;
    IdleStates->Update               = FALSE;
    IdleStates->InterfaceVersion     = 1;

    /* Callbacks */
    IdleStates->IdlePrepare          = AmdIdlePrepare;
    IdleStates->IdleCancel           = AmdIdleCancel;
    IdleStates->IdlePreselect        = AmdIdlePreselect;
    IdleStates->IdleTest             = AmdIdleTest;
    IdleStates->IdleAvailabilityCheck= AmdIdleAvailabilityCheck;
    IdleStates->IdlePreExecute       = AmdIdlePreExecute;
    IdleStates->IdleExecute          = AmdIdleExecute;
    IdleStates->IdleComplete         = AmdIdleComplete;
    IdleStates->IdleIsHalted         = AmdIdleIsHalted;
    IdleStates->IdleInitiateWake     = AmdIdleInitiateWake;

    IdleStates->MaximumDependencies  = 0;
    IdleStates->ProcessorIdleCount   = NumStates;

    /* Populate per-state entries */
    for (i = 0; i < NumStates; i++)
    {
        PPROCESSOR_IDLE_STATE_EX State = &IdleStates->State[i];

        if (DevExt->CST && i < DevExt->CST->Count)
        {
            ULONG CstType = DevExt->CST->States[i].Type;

            State->Flags.CStateType            = (CstType <= 7) ? CstType : 1;
            State->Flags.Interruptible         = (CstType <= 1) ? 1 : 0;
            State->Flags.CacheCoherent         = (CstType <= 2) ? 1 : 0;
            State->Flags.ThreadContextRetained = (CstType <= 1) ? 1 : 0;
            State->Flags.WakesSpuriously       = 0;
            State->Flags.PlatformOnly          = 0;
            State->Flags.NoCState              = 0;
            State->Flags.InterruptsEnabled     = (CstType == 1) ? 1 : 0;

            State->Latency          = DevExt->CST->States[i].Latency;
            State->BreakEvenDuration= DevExt->CST->States[i].Latency * 2;
            State->Power            = DevExt->CST->States[i].Power;
        }
        else
        {
            /*
             * Synthetic C1: interruptible, cache-coherent, 1µs latency,
             * contexts are retained (HLT equivalent).
             */
            State->Flags.CStateType            = 1;
            State->Flags.Interruptible         = 1;
            State->Flags.CacheCoherent         = 1;
            State->Flags.ThreadContextRetained = 1;
            State->Flags.InterruptsEnabled     = 1;
            State->Latency                     = 1;
            State->BreakEvenDuration           = 10;
            State->Power                       = 0;
        }
    }

    /*
     * Hand off to the kernel PPM dispatch table.
     * The kernel stores IdleStates in Prcb->PowerState.IdleState.
     * The memory must remain valid for the lifetime of the processor device;
     * the caller does not free it (it lives in NonPagedPool).
     */
    Status = AmdPpmGlobals.PpmDispatchTable.RegisterIdleStates(IdleStates);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: RegisterIdleStates dispatch failed: 0x%08lx\n", Status);
        ExFreePoolWithTag(IdleStates, TAG_AMDPPM_GENERIC);
        return Status;
    }

    DPRINT("AmdPpm: RegisterKernelIdleStates: %lu C-state(s) registered for CPU %u\n",
           NumStates, KeGetCurrentProcessorNumber());

    return STATUS_SUCCESS;
}

/*
 * RegisterKernelPerfStates
 *
 * Builds a PROCESSOR_PERF_STATES from the ACPI _PSS/_PCT tables stored in
 * DevExt and registers it with the kernel via the PPM dispatch table.
 *
 * The handler callbacks (PerfSelectionHandler, PerfControlHandler) are
 * filled with the FFH or ACPI I/O port transition functions depending on
 * what the CPUID analysis found in DrvCapabilities.
 */
NTSTATUS
RegisterKernelPerfStates(
    _In_ PFDO_DATA DevExt)
{
    NTSTATUS Status;
    PPROCESSOR_PERF_STATES PerfStates;
    PPROCESSOR_PERF_INFO   ProcInfo;

    PAGED_CODE();

    PerfStates = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(PROCESSOR_PERF_STATES),
                                       TAG_AMDPPM_GENERIC);
    if (!PerfStates)
        return STATUS_INSUFFICIENT_RESOURCES;

    ProcInfo = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(PROCESSOR_PERF_INFO),
                                     TAG_AMDPPM_GENERIC);
    if (!ProcInfo)
    {
        ExFreePoolWithTag(PerfStates, TAG_AMDPPM_GENERIC);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(PerfStates, sizeof(*PerfStates));
    RtlZeroMemory(ProcInfo,   sizeof(*ProcInfo));

    /* Identify the processor domain (single-CPU domain for now) */
    ProcInfo->InitialApicId  = DevExt->InitialApicId;
    ProcInfo->ProcessorIndex = DevExt->NtNumber;

    /* Fill in the PROCESSOR_PERF_STATES header */
    PerfStates->Version             = PROCESSOR_PERF_STATES_VERSION;
    PerfStates->ProcessorCount      = 1;
    PerfStates->Processors          = ProcInfo;
    PerfStates->MaxPerfPercent      = 100;
    PerfStates->MinPerfPercent      = 0;
    PerfStates->MinThrottlePercent  = 0;

    /* Determine the P-state type from capabilities */
    if (DevExt->DrvCapabilities & AMD_CAP_FFH)
    {
        PerfStates->Type            = PPM_PERF_STATE_TYPE_ACPI_FFH;
        DPRINT("AmdPpm: RegisterKernelPerfStates: using FFH P-states\n");
    }
    else
    {
        PerfStates->Type            = PPM_PERF_STATE_TYPE_ACPI_IO;
        DPRINT("AmdPpm: RegisterKernelPerfStates: using ACPI I/O P-states\n");
    }

    /* Extract the nominal frequency from _PSS entry 0 (highest P-state) */
    if (DevExt->PSS && DevExt->PSS->Count > 0)
    {
        PerfStates->NominalFrequency = DevExt->PSS->States[0].CoreFrequency;
    }

    /*
     * Apply _PPC cap if present.
     * PlatformCap is expressed as a percentage of max performance.
     */
    if ((DevExt->PPMEnabled & AMD_CAP_PPC) && DevExt->PSS && DevExt->PSS->Count > 0)
    {
        ULONG MaxIndex = DevExt->PSS->Count - 1;
        ULONG CapIndex = (DevExt->PPC_Cap <= MaxIndex) ? DevExt->PPC_Cap : MaxIndex;
        ULONG CapFreq  = DevExt->PSS->States[CapIndex].CoreFrequency;

        if (PerfStates->NominalFrequency > 0)
        {
            PerfStates->MaxPerfPercent =
                (CapFreq * 100) / PerfStates->NominalFrequency;
        }

        PerfStates->HardPlatformCap = TRUE;
        DPRINT("AmdPpm: PPC cap → MaxPerfPercent = %lu%%\n",
               PerfStates->MaxPerfPercent);
    }

    /* Affinity: all processors in this domain (single processor for now) */
    KeInitializeAffinityEx(&PerfStates->TargetProcessors);
    KeAddProcessorAffinityEx(&PerfStates->TargetProcessors,
                              DevExt->NtNumber);

    /*
     * Register with the kernel.
     * The kernel stores the pointer in Prcb->PowerState.PerfStates.
     * The memory must remain valid for the lifetime of this processor device.
     */
    Status = AmdPpmGlobals.PpmDispatchTable.RegisterPerfStates(PerfStates);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: RegisterPerfStates dispatch failed: 0x%08lx\n", Status);
        ExFreePoolWithTag(ProcInfo,    TAG_AMDPPM_GENERIC);
        ExFreePoolWithTag(PerfStates,  TAG_AMDPPM_GENERIC);
        return Status;
    }

    DPRINT("AmdPpm: RegisterKernelPerfStates: registered for CPU %lu "
           "(nom=%lu MHz, max=%lu%%)\n",
           DevExt->NtNumber,
           PerfStates->NominalFrequency,
           PerfStates->MaxPerfPercent);

    return STATUS_SUCCESS;
}

/*
 * RegisterKernelPerfCap
 *
 * Builds a PROCESSOR_CAP from the current PPC / TPC caps and registers it
 * with the kernel so the throttle policy engine is aware of the limits.
 *
 * Called:
 *   • Once during ProcLibDeviceStart (initial registration)
 *   • Again from EvtDeviceD0Entry on resume if the cap has changed
 */
NTSTATUS
RegisterKernelPerfCap(
    _In_ PFDO_DATA DevExt)
{
    NTSTATUS Status;
    PROCESSOR_CAP Cap;

    PAGED_CODE();

    RtlZeroMemory(&Cap, sizeof(Cap));

    Cap.Version                 = PROCESSOR_CAP_VERSION;
    Cap.ProcessorNumber.Group   = 0;
    Cap.ProcessorNumber.Number  = (UCHAR)DevExt->NtNumber;

    /*
     * PlatformCap: the _PPC value (0 = full speed, N = limit to P-state N).
     * Convert to a 0-100 percentage: 0 means full performance (100%),
     * higher _PPC values lower the cap.
     */
    if ((DevExt->PPMEnabled & AMD_CAP_PPC) &&
        DevExt->PSS && DevExt->PSS->Count > 0)
    {
        ULONG CapIndex = DevExt->PPC_Cap;
        ULONG MaxIndex = DevExt->PSS->Count - 1;

        if (CapIndex > MaxIndex)
            CapIndex = MaxIndex;

        if (DevExt->PSS->States[0].CoreFrequency > 0)
        {
            Cap.PlatformCap =
                (DevExt->PSS->States[CapIndex].CoreFrequency * 100) /
                DevExt->PSS->States[0].CoreFrequency;
        }
        else
        {
            Cap.PlatformCap = 100;
        }
    }
    else
    {
        Cap.PlatformCap = 100; /* No cap */
    }

    /*
     * ThermalCap: derived from TPC (throttle cap).
     * A TPC value of 0 means no throttling (100% performance).
     * Higher TPC values indicate heavier thermal throttling.
     */
    if ((DevExt->PPMEnabled & AMD_CAP_TSS) &&
        DevExt->TSS && DevExt->TSS->Count > 0)
    {
        ULONG CapIndex = DevExt->TPC_Cap;
        ULONG MaxIndex = DevExt->TSS->Count - 1;

        if (CapIndex > MaxIndex)
            CapIndex = MaxIndex;

        Cap.ThermalCap = DevExt->TSS->States[CapIndex].Percent;
    }
    else
    {
        Cap.ThermalCap = 100; /* No throttle cap */
    }

    Cap.LimitReasons = 0;

    Status = AmdPpmGlobals.PpmDispatchTable.RegisterPerfCap(&Cap);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: RegisterPerfCap dispatch failed: 0x%08lx\n", Status);
        return Status;
    }

    DPRINT("AmdPpm: RegisterKernelPerfCap: CPU %lu → PlatformCap=%lu%% ThermalCap=%lu%%\n",
           DevExt->NtNumber, Cap.PlatformCap, Cap.ThermalCap);

    return STATUS_SUCCESS;
}

/* PRIVATE HELPERS ***********************************************************/

/*
 * ProcLibDeviceCreate
 *
 * Called immediately after the WDFDEVICE is created (in EvtDriverDeviceAdd).
 * Initialises the FDO_DATA to a known state so PrepareHardware can populate it.
 */
NTSTATUS
ProcLibDeviceCreate(
    _In_ PFDO_DATA DevExt)
{
    PAGED_CODE();

    RtlZeroMemory(DevExt, sizeof(FDO_DATA));
    InitializeListHead(&DevExt->DeviceLink);

    DPRINT("AmdPpm: ProcLibDeviceCreate – device extension initialised\n");

    return STATUS_SUCCESS;
}

/*
 * ProcLibDeviceStart
 *
 * Called from EvtDevicePrepareHardware after the WDM device objects are known.
 * Acquires the ACPI interface, enumerates ACPI control methods, and registers
 * the available C/P/T-state capabilities with the kernel power manager.
 */
NTSTATUS
ProcLibDeviceStart(
    _In_ PFDO_DATA DevExt)
{
    NTSTATUS Status;
    ULONG FeaturesPresent = 0;

    PAGED_CODE();

    DPRINT("AmdPpm: ProcLibDeviceStart – CPU NtNumber=%lu\n", DevExt->NtNumber);

    /* Acquire the ACPI interface from the PDO stack */
    Status = AcquireAcpiInterfaces(DevExt);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: AcquireAcpiInterfaces failed (0x%08lx)\n", Status);
        /*
         * Continue without ACPI – fall back to CPUID-only capability
         * detection.  Some platforms expose P-states via MSRs (FFH) without
         * needing the ACPI namespace methods.
         */
    }

    /* Determine which ACPI namespace methods are present */
    Status = EnumerateControlMethods(DevExt, &FeaturesPresent);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: EnumerateControlMethods failed (0x%08lx)\n", Status);
        /* Non-fatal; proceed with whatever was found */
        Status = STATUS_SUCCESS;
    }

    DevExt->PPMFound = FeaturesPresent;

    DPRINT("AmdPpm: Features found: 0x%016llx\n", DevExt->PPMFound);

    /*
     * Evaluate _OSC to announce OS capabilities to the firmware.
     * Ignore failures – older BIOSes may not implement _OSC.
     */
    if (FeaturesPresent & AMD_CAP_OSC)
    {
        OSC_INPUT_BUFFER OscInput;
        OSC_OUTPUT_BUFFER *OscOutput = NULL;

        /* Processor Aggregator Device _OSC UUID: {0811B06E-4A27-44F9-8D60-3CBBC22E7B48} */
        static const UCHAR ProcessorOscUuid[16] = {
            0x6E, 0xB0, 0x11, 0x08, 0x27, 0x4A, 0xF9, 0x44,
            0x8D, 0x60, 0x3C, 0xBB, 0xC2, 0x2E, 0x7B, 0x48
        };

        RtlCopyMemory(OscInput.Uuid, ProcessorOscUuid, 16);
        OscInput.Revision = 1;
        OscInput.Count = 2;
        OscInput.Capabilities[0] = 0;   /* Query only (bit 0 clear) */
        OscInput.Capabilities[1] = 0;   /* No specific capabilities requested */

        Status = AcpiEval_OSC(DevExt, &OscInput, sizeof(OscInput), &OscOutput);
        if (NT_SUCCESS(Status) && OscOutput)
        {
            DevExt->OscOutput = OscOutput;
            DPRINT("AmdPpm: _OSC returned status 0x%08lx, caps[0]=0x%08lx\n",
                   OscOutput->Status, OscOutput->Capabilities[0]);
        }
        Status = STATUS_SUCCESS; /* Non-fatal */
    }

    /*
     * Evaluate _PDC (Processor Driver Capabilities) to advertise what the OS
     * supports to the firmware.  This is used on older ACPI 2.0 systems that
     * do not have _OSC.
     */
    if (FeaturesPresent & AMD_CAP_PDC)
    {
        /* PDC capabilities: ACPI 3.0 C-states, P-states, T-states */
        struct {
            PDC_INPUT_BUFFER Header;
            ULONG ExtraCaps;
        } PdcBuf;

        PdcBuf.Header.Revision  = 1;
        PdcBuf.Header.Count     = 2;
        PdcBuf.Header.Capabilities[0] =
            0x00000001 |  /* C1 power state support                  */
            0x00000002 |  /* C2 power state support                  */
            0x00000004 |  /* C3 power state support                  */
            0x00000008 |  /* P-state support (ACPI 3.0)              */
            0x00000010 |  /* T-state support                         */
            0x00000020;   /* Processor capacity sharing              */
        PdcBuf.ExtraCaps = 0;

        Status = AcpiEval_PDC(DevExt, &PdcBuf.Header, sizeof(PdcBuf));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("AmdPpm: _PDC failed (0x%08lx) – continuing\n", Status);
        }
        Status = STATUS_SUCCESS;
    }

    /*
     * Read the C-state table (_CST) and register it with the kernel.
     */
    if (FeaturesPresent & AMD_CAP_CST)
    {
        ACPI_CST *Cst = NULL;

        Status = AcpiEval_CST(DevExt, &Cst);
        if (NT_SUCCESS(Status) && Cst)
        {
            DevExt->CST = Cst;
            DevExt->PPMEnabled |= AMD_CAP_CST;

            DPRINT("AmdPpm: _CST: %lu C-state(s) found\n", Cst->Count);
        }
        Status = STATUS_SUCCESS;
    }

    /*
     * Register C-states (idle states) with the kernel.
     * RegisterKernelIdleStates builds PROCESSOR_IDLE_STATES_EX from the
     * ACPI_CST table (or a synthetic C1 entry) and calls the dispatch table.
     */
    if (AmdPpmGlobals.RegisterIdleStates)
    {
        Status = AmdPpmGlobals.RegisterIdleStates(DevExt);
        if (NT_SUCCESS(Status))
        {
            DevExt->PPMEnabled |= AMD_CAP_CST;
        }
        else
        {
            DPRINT1("AmdPpm: RegisterIdleStates failed: 0x%08lx – continuing\n",
                    Status);
        }
        Status = STATUS_SUCCESS;
    }

    /*
     * Read the P-state tables (_PSS / _XPSS, _PCT) and register them.
     */
    if (FeaturesPresent & AMD_CAP_PSS)
    {
        ACPI_PSS *Pss = NULL;

        Status = AcpiEval_PSS(DevExt, &Pss);
        if (NT_SUCCESS(Status) && Pss)
        {
            DevExt->PSS = Pss;
            DevExt->PPMEnabled |= AMD_CAP_PSS;
            DPRINT("AmdPpm: _PSS: %lu P-state(s) found\n", Pss->Count);
        }

        /* Also try _PCT for the control/status register addresses */
        if (FeaturesPresent & AMD_CAP_PCT)
        {
            Status = AcpiEval_PCT_PTC(DevExt, ACPI_METHOD_PCT, &DevExt->PCT);
            if (NT_SUCCESS(Status))
            {
                DevExt->PPMEnabled |= AMD_CAP_PCT;
                DPRINT("AmdPpm: _PCT acquired (AddrSpaceID=%u)\n",
                       DevExt->PCT.Control.AddressSpaceID);

                /* Validate that the PSS table is usable with this PCT */
                if (Pss)
                {
                    ULONG ValidationErrors = 0;
                    Status = ValidatePStateCapability(&DevExt->PCT, Pss,
                                                     &ValidationErrors);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("AmdPpm: P-state cap validation failed: 0x%08lx "
                                "(errors=0x%lx)\n", Status, ValidationErrors);
                    }
                }
            }
        }

        Status = STATUS_SUCCESS;
    }

    /*
     * Register P-states (performance states) with the kernel.
     * RegisterKernelPerfStates builds PROCESSOR_PERF_STATES from the ACPI
     * _PSS/_PCT tables and calls the dispatch table.
     */
    if ((DevExt->PPMEnabled & AMD_CAP_PSS) && AmdPpmGlobals.RegisterPStates)
    {
        Status = AmdPpmGlobals.RegisterPStates(DevExt);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("AmdPpm: RegisterPStates failed: 0x%08lx – continuing\n",
                    Status);
        }
        Status = STATUS_SUCCESS;
    }

    /*
     * Register initial performance capability with the kernel.
     */
    if (AmdPpmGlobals.RegisterPerfCap)
    {
        Status = AmdPpmGlobals.RegisterPerfCap(DevExt);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("AmdPpm: RegisterPerfCap failed: 0x%08lx – continuing\n",
                    Status);
        }
        Status = STATUS_SUCCESS;
    }

    /*
     * Read _PSD (P-state dependency) for P-state coordination.
     */
    if (FeaturesPresent & AMD_CAP_PSD)
    {
        ACPI_XSD *Psd = NULL;
        Status = AcpiEval_PSD_TSD(DevExt, ACPI_METHOD_PSD, &Psd);
        if (NT_SUCCESS(Status) && Psd)
        {
            DevExt->PSD = Psd;
            DevExt->PPMEnabled |= AMD_CAP_PSD;
            DPRINT("AmdPpm: _PSD: %lu domain(s)\n", Psd->Count);
        }
        Status = STATUS_SUCCESS;
    }

    /*
     * Read _PPC (performance present capabilities = max P-state index).
     */
    if (FeaturesPresent & AMD_CAP_PPC)
    {
        Status = AcpiEval_PPC(DevExt, &DevExt->PPC);
        if (NT_SUCCESS(Status))
        {
            DevExt->PPC_Cap = DevExt->PPC;
            DevExt->PPMEnabled |= AMD_CAP_PPC;
            DPRINT("AmdPpm: _PPC cap = %lu\n", DevExt->PPC);
        }
        Status = STATUS_SUCCESS;
    }

    /*
     * T-state support (_TSS, _PTC, _TSD, _TPC).
     */
    if (FeaturesPresent & AMD_CAP_TSS)
    {
        ACPI_TSS *Tss = NULL;
        Status = AcpiEval_TSS(DevExt, &Tss);
        if (NT_SUCCESS(Status) && Tss)
        {
            DevExt->TSS = Tss;
            DevExt->PPMEnabled |= AMD_CAP_TSS;
            DPRINT("AmdPpm: _TSS: %lu T-state(s)\n", Tss->Count);
        }

        if (FeaturesPresent & AMD_CAP_PCT) /* _PTC uses same PCT field by convention */
        {
            Status = AcpiEval_PCT_PTC(DevExt, ACPI_METHOD_PTC, &DevExt->PTC);
            if (NT_SUCCESS(Status))
            {
                DevExt->PPMEnabled |= (AMD_CAP_TSS & ~AMD_CAP_TSS) | AMD_CAP_TSS;
            }
        }

        Status = STATUS_SUCCESS;
    }

    DPRINT("AmdPpm: ProcLibDeviceStart complete – enabled=0x%016llx\n",
           DevExt->PPMEnabled);

    return STATUS_SUCCESS;
}

/* WDF CALLBACK IMPLEMENTATIONS **********************************************/

/*
 * EvtDevicePrepareHardware
 *
 * Called by WDF when the device is about to enter D0 for the first time.
 * At this point the PDO is accessible and ACPI is functional.
 */
NTSTATUS
STDCALL
EvtDevicePrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    PFDO_DATA DevExt;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    DevExt = AmdPpmGetFdoData(Device);

    /*
     * Store the WDM device objects in our context so that helper functions
     * (especially AcquireAcpiInterfaces) can use them.
     */
    DevExt->Self          = WdfDeviceWdmGetDeviceObject(Device);
    DevExt->Pdo           = WdfDeviceWdmGetPhysicalDevice(Device);
    DevExt->DefaultTarget = WdfDeviceGetIoTarget(Device);

    return ProcLibDeviceStart(DevExt);
}

/*
 * EvtDeviceReleaseHardware
 *
 * Called when the device is removed or the driver is unloaded.
 * Free all heap-allocated ACPI tables.
 */
NTSTATUS
STDCALL
EvtDeviceReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    PFDO_DATA DevExt;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    DevExt = AmdPpmGetFdoData(Device);

    /*
     * Drop the object-manager reference on the PDO taken in
     * AcquireAcpiInterfaces during EvtDevicePrepareHardware.
     */
    ReleaseAcpiInterfaces(DevExt);

    /* Free ACPI-allocated tables */
    if (DevExt->OscOutput)
    {
        ExFreePoolWithTag(DevExt->OscOutput, TAG_AMDPPM_ACPI);
        DevExt->OscOutput = NULL;
    }
    if (DevExt->PSS)
    {
        ExFreePoolWithTag(DevExt->PSS, TAG_AMDPPM_PSS);
        DevExt->PSS = NULL;
    }
    if (DevExt->XPSS)
    {
        ExFreePoolWithTag(DevExt->XPSS, TAG_AMDPPM_PSS);
        DevExt->XPSS = NULL;
    }
    if (DevExt->CST)
    {
        ExFreePoolWithTag(DevExt->CST, TAG_AMDPPM_CST);
        DevExt->CST = NULL;
    }
    if (DevExt->TSS)
    {
        ExFreePoolWithTag(DevExt->TSS, TAG_AMDPPM_TSS);
        DevExt->TSS = NULL;
    }
    if (DevExt->PSD)
    {
        ExFreePoolWithTag(DevExt->PSD, TAG_AMDPPM_ACPI);
        DevExt->PSD = NULL;
    }
    if (DevExt->TSD)
    {
        ExFreePoolWithTag(DevExt->TSD, TAG_AMDPPM_ACPI);
        DevExt->TSD = NULL;
    }

    /*
     * Remove from the global device list.
     */
    WdfWaitLockAcquire(AmdPpmGlobals.Mutex, NULL);
    if (DevExt->DeviceLink.Flink != NULL && !IsListEmpty(&DevExt->DeviceLink))
    {
        RemoveEntryList(&DevExt->DeviceLink);
    }
    WdfWaitLockRelease(AmdPpmGlobals.Mutex);

    return STATUS_SUCCESS;
}

/*
 * EvtDeviceD0Entry
 *
 * Called on every D0 entry (including resume from sleep).
 * On resume, re-read _PPC / _TPC as the firmware may have lowered caps.
 */
NTSTATUS
STDCALL
EvtDeviceD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    PFDO_DATA DevExt;
    ULONG NewPpc = 0, NewTpc = 0;

    PAGED_CODE();

    DevExt = AmdPpmGetFdoData(Device);

    /*
     * On a sleep/resume cycle (S1-S4), ACPI notifications 0x80 (PPC) and
     * 0x81 (TPC) may have been missed.  Re-evaluate _PPC and _TPC to make
     * sure caps are current.
     */
    if (PreviousState != WdfPowerDeviceD3Final &&
        PreviousState != WdfPowerDeviceInvalid)
    {
        DevExt->ResumeFromSleep = TRUE;

        if ((DevExt->PPMEnabled & AMD_CAP_PPC) &&
            NT_SUCCESS(AcpiEval_PPC(DevExt, &NewPpc)))
        {
            if (NewPpc != DevExt->PPC_Cap)
            {
                DPRINT("AmdPpm: PPC changed %lu → %lu on resume\n",
                       DevExt->PPC_Cap, NewPpc);
                DevExt->PPC_Cap = NewPpc;

                /*
                 * Notify the kernel of the updated platform performance cap
                 * so the throttle policy engine applies the new limit.
                 */
                if (AmdPpmGlobals.RegisterPerfCap)
                    AmdPpmGlobals.RegisterPerfCap(DevExt);
            }
        }

        if ((DevExt->PPMEnabled & AMD_CAP_TSS) &&
            NT_SUCCESS(AcpiEval_TPC(DevExt, &NewTpc)))
        {
            if (NewTpc != DevExt->TPC_Cap)
            {
                DPRINT("AmdPpm: TPC changed %lu → %lu on resume\n",
                       DevExt->TPC_Cap, NewTpc);
                DevExt->TPC_Cap = NewTpc;

                /*
                 * Notify the kernel of the updated thermal throttle cap.
                 */
                if (AmdPpmGlobals.RegisterPerfCap)
                    AmdPpmGlobals.RegisterPerfCap(DevExt);
            }
        }
    }

    return STATUS_SUCCESS;
}

/*
 * EvtDriverUnload
 *
 * Called when the driver is being unloaded (e.g. during system shutdown).
 */
VOID
STDCALL
EvtDriverUnload(
    _In_ WDFDRIVER Driver)
{
    UNREFERENCED_PARAMETER(Driver);

    DPRINT("AmdPpm: EvtDriverUnload\n");

    ProcLibDriverCleanup();
}

/*
 * EvtDriverDeviceAdd
 *
 * Called by WDF once for each processor device object the PnP manager
 * presents to this driver.  Creates the WDFDEVICE and registers the
 * PnP/power callbacks.
 */
NTSTATUS
STDCALL
EvtDriverDeviceAdd(
    _In_ WDFDRIVER Driver,
    _In_ PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS Status;
    WDFDEVICE DeviceHandle;
    PFDO_DATA DevExt;
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES DeviceAttributes;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(Driver);

    /* Register PnP/power event callbacks */
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerCallbacks);
    PnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    PnpPowerCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
    PnpPowerCallbacks.EvtDeviceD0Entry         = EvtDeviceD0Entry;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPowerCallbacks);

    /*
     * The processor driver is not the power policy owner; power is managed
     * by the system power manager via the PPM dispatch table.
     */
    WdfDeviceInitSetPowerPolicyOwnership(DeviceInit, FALSE);

    /*
     * Allocate FDO_DATA as WDF context data on the device object.
     */
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&DeviceAttributes, FDO_DATA);
    DeviceAttributes.ExecutionLevel      = WdfExecutionLevelPassive;
    DeviceAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

    Status = WdfDeviceCreate(&DeviceInit, &DeviceAttributes, &DeviceHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: WdfDeviceCreate failed: 0x%08lx\n", Status);
        return Status;
    }

    DevExt = AmdPpmGetFdoData(DeviceHandle);

    /* Basic initialisation; full init happens in EvtDevicePrepareHardware */
    Status = ProcLibDeviceCreate(DevExt);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: ProcLibDeviceCreate failed: 0x%08lx\n", Status);
        return Status;
    }

    /*
     * Register the processor device interface so that power management tools
     * can discover this driver.
     *
     * FIXME: Register GUID_DEVINTERFACE_PROCESSOR here once the GUID is
     *        confirmed available in ReactOS DDK.
     */

    DPRINT("AmdPpm: EvtDriverDeviceAdd – device created\n");

    return STATUS_SUCCESS;
}

/* GLOBAL INIT / CLEANUP *****************************************************/

/*
 * ProcLibDriverCleanup
 *
 * Releases any global resources allocated by ProcLibGlobalInit.
 * Called from EvtDriverUnload and on error paths in DriverEntry.
 */
NTSTATUS
ProcLibDriverCleanup(
    VOID)
{
    PAGED_CODE();

    DPRINT("AmdPpm: ProcLibDriverCleanup\n");

    /*
     * The WDF wait lock (AmdPpmGlobals.Mutex) is a WDF object; it will be
     * cleaned up automatically when the WDFDRIVER object is deleted.
     */

    return STATUS_SUCCESS;
}

/*
 * ProcLibGlobalInit
 *
 * Driver-wide initialisation performed after WdfDriverCreate succeeds.
 *
 * Steps:
 *  1. Query the kernel PPM dispatch table via ZwPowerInformation.
 *  2. Verify the interface version.
 *  3. Initialise synchronisation objects and list heads.
 *  4. Read registry overrides.
 *  5. Set up global function pointer table (RegisterIdleStates, etc.).
 */
NTSTATUS
ProcLibGlobalInit(
    _In_ PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status;
    ULONG OverrideFlags = 0;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    WDF_OBJECT_ATTRIBUTES LockAttributes;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);

    RtlZeroMemory(&AmdPpmGlobals, sizeof(AmdPpmGlobals));

    /* ------------------------------------------------------------------ */
    /* Step 1 & 2: Retrieve the PPM dispatch table from the kernel         */
    /* ------------------------------------------------------------------ */
    Status = ZwPowerInformation(
                 ProcessorStateHandler,
                 NULL, 0,
                 &AmdPpmGlobals.PpmDispatchTable,
                 sizeof(PPM_DRIVER_DISPATCH_TABLE));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: ZwPowerInformation(ProcessorStateHandler) failed: "
                "0x%08lx\n", Status);
        return Status;
    }

    if (AmdPpmGlobals.PpmDispatchTable.InterfaceVersion !=
        PPM_DRIVER_INTERFACE_VERSION)
    {
        DPRINT1("AmdPpm: Kernel PPM interface version mismatch: "
                "got %lu, expected %u\n",
                AmdPpmGlobals.PpmDispatchTable.InterfaceVersion,
                PPM_DRIVER_INTERFACE_VERSION);
        return STATUS_NOT_SUPPORTED;
    }

    DPRINT("AmdPpm: PPM dispatch table acquired (version %lu)\n",
           AmdPpmGlobals.PpmDispatchTable.InterfaceVersion);

    /* ------------------------------------------------------------------ */
    /* Step 3: Synchronisation objects and list heads                       */
    /* ------------------------------------------------------------------ */
    WDF_OBJECT_ATTRIBUTES_INIT(&LockAttributes);
    Status = WdfWaitLockCreate(&LockAttributes, &AmdPpmGlobals.Mutex);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: WdfWaitLockCreate failed: 0x%08lx\n", Status);
        return Status;
    }

    InitializeListHead(&AmdPpmGlobals.DeviceHead);

    /* Query the number of active logical processors */
    AmdPpmGlobals.ProcessorCount =
        KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

    /* ------------------------------------------------------------------ */
    /* Step 4: Registry overrides                                           */
    /* ------------------------------------------------------------------ */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT |
                                  RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[0].Name          = L"Overrides";
    QueryTable[0].EntryContext  = &OverrideFlags;
    QueryTable[0].DefaultType   = REG_DWORD;
    QueryTable[0].DefaultData   = &OverrideFlags;
    QueryTable[0].DefaultLength = sizeof(OverrideFlags);

    RtlQueryRegistryValues(
        RTL_REGISTRY_ABSOLUTE,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Processor",
        QueryTable, NULL, NULL);

    AmdPpmGlobals.PPMOverrideFlags = OverrideFlags;

    /* ------------------------------------------------------------------ */
    /* Step 5: Global function pointers                                     */
    /* ------------------------------------------------------------------ */
    /*
     * Point the global dispatch stubs to the per-device implementations.
     * ProcLibDeviceStart (called from EvtDevicePrepareHardware) uses these
     * to register C/P/T-state capability tables with the kernel.
     */
    AmdPpmGlobals.RegisterIdleStates = RegisterKernelIdleStates;
    AmdPpmGlobals.RegisterPStates    = RegisterKernelPerfStates;
    AmdPpmGlobals.RegisterPerfCap    = RegisterKernelPerfCap;

    DPRINT("AmdPpm: ProcLibGlobalInit complete (%lu processor(s))\n",
           AmdPpmGlobals.ProcessorCount);

    return STATUS_SUCCESS;
}

/* DRIVER ENTRY **************************************************************/

/*
 * DriverEntry
 *
 * Entry point called by the kernel when the driver is loaded.
 *
 * Sequence:
 *   1. Verify we are running on an AMD processor (CPUID).
 *   2. Create the WDF driver object.
 *   3. Call ProcLibGlobalInit to contact the kernel PPM subsystem.
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    WDF_DRIVER_CONFIG DriverConfig;

    DPRINT("AmdPpm: DriverEntry\n");

    /*
     * Bail out early if this is not an AMD processor.
     * (This prevents the driver being loaded on Intel/ARM hardware.)
     */
    if (!IsAmdProcessor())
    {
        DPRINT1("AmdPpm: Not an AMD processor – aborting load\n");
        return STATUS_NOT_SUPPORTED;
    }

    /* Initialise WDF driver configuration */
    WDF_DRIVER_CONFIG_INIT(&DriverConfig, EvtDriverDeviceAdd);
    DriverConfig.EvtDriverUnload = EvtDriverUnload;

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &DriverConfig,
                             WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: WdfDriverCreate failed: 0x%08lx\n", Status);
        return Status;
    }

    /* Initialise global state and retrieve the kernel dispatch table */
    Status = ProcLibGlobalInit(DriverObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("AmdPpm: ProcLibGlobalInit failed: 0x%08lx\n", Status);
        ProcLibDriverCleanup();
        return Status;
    }

    DPRINT("AmdPpm: DriverEntry – driver loaded successfully\n");

    return STATUS_SUCCESS;
}

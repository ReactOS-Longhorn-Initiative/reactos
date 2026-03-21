/*
 * PROJECT:     ReactOS Generic CPU Driver — ACPI _PSS / _PCT for Intel PPM
 * LICENSE:     GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:     Evaluate processor performance objects via IOCTL_ACPI_EVAL_METHOD
 *              (same pattern as amdppm acpi.c).
 */

#include "processr.h"

#include <acpiioct.h>

#define NDEBUG
#include <debug.h>

#define TAG_PROCESSR_ACPI  'icPA'

#define ACPI_METHOD_NAME_4(a, b, c, d)                                      \
    ((ULONG)(UCHAR)(a) | ((ULONG)(UCHAR)(b) << 8) |                         \
     ((ULONG)(UCHAR)(c) << 16) | ((ULONG)(UCHAR)(d) << 24))

#define ACPI_METHOD_PSS   ACPI_METHOD_NAME_4('_', 'P', 'S', 'S')
#define ACPI_METHOD_XPSS  ACPI_METHOD_NAME_4('X', 'P', 'S', 'S')
#define ACPI_METHOD_PCT   ACPI_METHOD_NAME_4('_', 'P', 'C', 'T')

#define ACPI_ADR_SPACE_FIXED_HW  0x7F

static BOOLEAN
ProcessrAcpiArgAsUlong(
    _In_ PACPI_METHOD_ARGUMENT Arg,
    _Out_ PULONG Value)
{
    if (!Arg || Arg->Type != ACPI_METHOD_ARGUMENT_INTEGER)
        return FALSE;

    *Value = Arg->Argument;
    return TRUE;
}

static PACPI_METHOD_ARGUMENT
ProcessrAcpiArgFirstInPackage(
    _In_ PACPI_METHOD_ARGUMENT Package,
    _Out_ PULONG PackageLength)
{
    if (!Package ||
        (Package->Type != ACPI_METHOD_ARGUMENT_PACKAGE &&
         Package->Type != ACPI_METHOD_ARGUMENT_PACKAGE_EX))
    {
        *PackageLength = 0;
        return NULL;
    }

    *PackageLength = Package->DataLength;
    return (PACPI_METHOD_ARGUMENT)(ULONG_PTR)Package->Data;
}

static NTSTATUS
ProcessrAcpiSendIoctl(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG IoControlCode,
    _In_opt_ PVOID InBuf,
    _In_ ULONG InBufLen,
    _Out_ PACPI_EVAL_OUTPUT_BUFFER *OutBuffer,
    _Out_ PULONG OutBufferLen)
{
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    PACPI_EVAL_OUTPUT_BUFFER OutBuf;
    ULONG OutSize;

    *OutBuffer = NULL;
    *OutBufferLen = 0;

    OutSize = sizeof(ACPI_EVAL_OUTPUT_BUFFER) + 64;
    OutBuf = ExAllocatePoolWithTag(PagedPool, OutSize, TAG_PROCESSR_ACPI);
    if (!OutBuf)
        return STATUS_INSUFFICIENT_RESOURCES;

Retry:
    RtlZeroMemory(OutBuf, OutSize);

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(
              IoControlCode,
              Pdo,
              InBuf,
              InBufLen,
              OutBuf,
              OutSize,
              FALSE,
              &Event,
              &IoStatus);

    if (!Irp)
    {
        ExFreePoolWithTag(OutBuf, TAG_PROCESSR_ACPI);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCallDriver(Pdo, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    if (Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW)
    {
        ULONG NewSize = OutBuf->Length;

        ExFreePoolWithTag(OutBuf, TAG_PROCESSR_ACPI);

        if (NewSize <= OutSize || NewSize > 0x10000)
            return STATUS_ACPI_INVALID_DATA;

        OutSize = NewSize;
        OutBuf = ExAllocatePoolWithTag(PagedPool, OutSize, TAG_PROCESSR_ACPI);
        if (!OutBuf)
            return STATUS_INSUFFICIENT_RESOURCES;

        goto Retry;
    }

    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(OutBuf, TAG_PROCESSR_ACPI);
        return Status;
    }

    if (OutBuf->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE || OutBuf->Count == 0)
    {
        ExFreePoolWithTag(OutBuf, TAG_PROCESSR_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    *OutBuffer = OutBuf;
    *OutBufferLen = OutSize;

    return STATUS_SUCCESS;
}

static NTSTATUS
ProcessrAcpiEvaluateMethod(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG MethodNameAsUlong,
    _Out_ PACPI_EVAL_OUTPUT_BUFFER *OutBuffer,
    _Out_ PULONG OutBufferLen)
{
    ACPI_EVAL_INPUT_BUFFER InBuf;

    RtlZeroMemory(&InBuf, sizeof(InBuf));
    InBuf.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    InBuf.MethodNameAsUlong = MethodNameAsUlong;

    return ProcessrAcpiSendIoctl(Pdo,
                                 IOCTL_ACPI_EVAL_METHOD,
                                 &InBuf,
                                 sizeof(InBuf),
                                 OutBuffer,
                                 OutBufferLen);
}

static NTSTATUS
ProcessrParsePssOutput(
    _In_ PACPI_EVAL_OUTPUT_BUFFER AcpiOut,
    _Out_ PPROCESSR_PSS *OutPss)
{
    PACPI_METHOD_ARGUMENT PkgArg;
    ULONG Count, i;
    PPROCESSR_PSS Pss;

    *OutPss = NULL;

    Count = AcpiOut->Count;
    if (Count == 0 || Count > 32)
        return STATUS_ACPI_INVALID_DATA;

    Pss = ExAllocatePoolWithTag(
              PagedPool,
              FIELD_OFFSET(PROCESSR_PSS, States) + Count * sizeof(PROCESSR_PSS_STATE),
              TAG_PROCESSR_ACPI);
    if (!Pss)
        return STATUS_INSUFFICIENT_RESOURCES;

    Pss->Count = Count;
    PkgArg = AcpiOut->Argument;

    for (i = 0; i < Count; i++)
    {
        ULONG PkgLen = 0;
        PACPI_METHOD_ARGUMENT Sub = ProcessrAcpiArgFirstInPackage(PkgArg, &PkgLen);

        if (Sub)
        {
            ProcessrAcpiArgAsUlong(Sub, &Pss->States[i].CoreFrequency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            ProcessrAcpiArgAsUlong(Sub, &Pss->States[i].Power);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            ProcessrAcpiArgAsUlong(Sub, &Pss->States[i].TransitionLatency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            ProcessrAcpiArgAsUlong(Sub, &Pss->States[i].BusMasterLatency);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            ProcessrAcpiArgAsUlong(Sub, &Pss->States[i].Control);
            Sub = ACPI_METHOD_NEXT_ARGUMENT(Sub);
            ProcessrAcpiArgAsUlong(Sub, &Pss->States[i].Status);
        }

        PkgArg = ACPI_METHOD_NEXT_ARGUMENT(PkgArg);
    }

    *OutPss = Pss;
    return STATUS_SUCCESS;
}

static NTSTATUS
ProcessrAcpiEvalPssTable(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG MethodUlong,
    _Out_ PPROCESSR_PSS *OutPss)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;

    *OutPss = NULL;

    Status = ProcessrAcpiEvaluateMethod(Pdo, MethodUlong, &AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ProcessrParsePssOutput(AcpiOut, OutPss);
    ExFreePoolWithTag(AcpiOut, TAG_PROCESSR_ACPI);

    return Status;
}

static NTSTATUS
ProcessrAcpiEvalPct(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ PPROCESSR_ACPI_CTRL_STATUS Pct)
{
    NTSTATUS Status;
    PACPI_EVAL_OUTPUT_BUFFER AcpiOut = NULL;
    ULONG AcpiOutLen = 0;
    PACPI_METHOD_ARGUMENT Arg;

    RtlZeroMemory(Pct, sizeof(*Pct));

    Status = ProcessrAcpiEvaluateMethod(Pdo, ACPI_METHOD_PCT, &AcpiOut, &AcpiOutLen);
    if (!NT_SUCCESS(Status))
        return Status;

    if (AcpiOut->Count < 2)
    {
        ExFreePoolWithTag(AcpiOut, TAG_PROCESSR_ACPI);
        return STATUS_ACPI_INVALID_DATA;
    }

    Arg = AcpiOut->Argument;

    if (Arg->Type == ACPI_METHOD_ARGUMENT_BUFFER &&
        Arg->DataLength >= sizeof(PROCESSR_GEN_ADDR))
    {
        RtlCopyMemory(&Pct->Control, Arg->Data, sizeof(PROCESSR_GEN_ADDR));
    }

    Arg = ACPI_METHOD_NEXT_ARGUMENT(Arg);

    if (Arg->Type == ACPI_METHOD_ARGUMENT_BUFFER &&
        Arg->DataLength >= sizeof(PROCESSR_GEN_ADDR))
    {
        RtlCopyMemory(&Pct->Status, Arg->Data, sizeof(PROCESSR_GEN_ADDR));
    }

    ExFreePoolWithTag(AcpiOut, TAG_PROCESSR_ACPI);
    return STATUS_SUCCESS;
}

VOID
ProcessrReleaseAcpiPerfData(
    _Inout_ PDEVICE_EXTENSION DevExt)
{
    if (DevExt->AcpiPctMemoryVirt)
    {
        MmUnmapIoSpace(DevExt->AcpiPctMemoryVirt, DevExt->AcpiPctMemoryBytes);
        DevExt->AcpiPctMemoryVirt = NULL;
        DevExt->AcpiPctMemoryBytes = 0;
    }

    if (DevExt->AcpiPss)
    {
        ExFreePoolWithTag(DevExt->AcpiPss, TAG_PROCESSR_ACPI);
        DevExt->AcpiPss = NULL;
    }

    DevExt->AcpiPctValid = FALSE;
    RtlZeroMemory(&DevExt->AcpiPct, sizeof(DevExt->AcpiPct));
}

NTSTATUS
NTAPI
ProcessrCollectAcpiPerfStates(
    _In_ PDEVICE_OBJECT ProcessorPdo,
    _Inout_ PDEVICE_EXTENSION DevExt)
{
    NTSTATUS Status;
    PPROCESSR_PSS Pss = NULL;

    PAGED_CODE();

    ProcessrReleaseAcpiPerfData(DevExt);

    if (!ProcessorPdo)
        return STATUS_INVALID_PARAMETER;

    Status = ProcessrAcpiEvalPssTable(ProcessorPdo, ACPI_METHOD_PSS, &Pss);
    if (!NT_SUCCESS(Status))
        Status = ProcessrAcpiEvalPssTable(ProcessorPdo, ACPI_METHOD_XPSS, &Pss);

    if (!NT_SUCCESS(Status) || !Pss || Pss->Count == 0)
    {
        if (Pss)
        {
            ExFreePoolWithTag(Pss, TAG_PROCESSR_ACPI);
            Pss = NULL;
        }
        return STATUS_NOT_FOUND;
    }

    DevExt->AcpiPss = Pss;

    Status = ProcessrAcpiEvalPct(ProcessorPdo, &DevExt->AcpiPct);
    if (NT_SUCCESS(Status))
        DevExt->AcpiPctValid = TRUE;
    else
        DevExt->AcpiPctValid = FALSE;

    DPRINT("Processr: ACPI P-states: %lu entries, _PCT %s\n",
           Pss->Count,
           DevExt->AcpiPctValid ? "ok" : "absent");

    return STATUS_SUCCESS;
}

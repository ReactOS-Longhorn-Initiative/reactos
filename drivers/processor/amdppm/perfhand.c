/*
 * PROJECT:     ReactOS AMD Processor Power Management Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        drivers/processor/amdppm/perfhand.c
 * PURPOSE:     Windows-style PerfSelectionHandler / PerfControlHandler for PPM.
 *
 * REFERENCES:  Windows 10 amdppm.sys (PerfSelectionPTStates / PerfControlPTStates)
 *
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 */

#include "amdppm.h"

#define AMDPPM_MAX_PROCESSORS 256

static ULONG_PTR
NTAPI
AmdPpmIpiClearPerfStatesOnTarget(
    _In_ ULONG_PTR Context)
{
    ULONG TargetCpu = (ULONG)Context;

    if (KeGetCurrentProcessorIndex() == TargetCpu &&
        AmdPpmGlobals.PpmDispatchTable.RegisterPerfStates != NULL)
    {
        (VOID)AmdPpmGlobals.PpmDispatchTable.RegisterPerfStates(NULL);
    }
    return 0;
}

static PFDO_DATA AmdPpmPerfFdoByProcessor[AMDPPM_MAX_PROCESSORS];

static __inline ULONG
AmdPpmMinUlong(
    _In_ ULONG A,
    _In_ ULONG B)
{
    return (A < B) ? A : B;
}

VOID
AmdPpmPerfRegisterProcessorFdo(
    _In_ PFDO_DATA DevExt)
{
    if (DevExt->NtNumber < AMDPPM_MAX_PROCESSORS)
        AmdPpmPerfFdoByProcessor[DevExt->NtNumber] = DevExt;
}

VOID
AmdPpmPerfUnregisterProcessorFdo(
    _In_ PFDO_DATA DevExt)
{
    if (DevExt->NtNumber < AMDPPM_MAX_PROCESSORS &&
        AmdPpmPerfFdoByProcessor[DevExt->NtNumber] == DevExt)
    {
        AmdPpmPerfFdoByProcessor[DevExt->NtNumber] = NULL;
    }
}

/*
 * Drop the kernel PPM registration for this logical processor on the correct
 * CPU, then caller may free KernelRegisteredPerfStates / ProcInfo.
 */
VOID
AmdPpmClearKernelPerfStatesRegistration(
    _In_ ULONG NtProcessorNumber)
{
    (VOID)KeIpiGenericCall(AmdPpmIpiClearPerfStatesOnTarget,
                           (ULONG_PTR)NtProcessorNumber);
}

ULONG
FASTCALL
AmdPpmPerfSelection(
    _In_ ULONG Context,
    _In_ ULONG TargetPercent,
    _In_ ULONG MinPercent,
    _In_ ULONG MaxPercent,
    _In_ ULONG Flags,
    _Out_ PULONG Frequency,
    _Out_ PULONGLONG Selection)
{
    PFDO_DATA DevExt;
    ULONG Cpu;
    ULONG maxIdx, slowest, idx;
    ULONG nominal, t;
    ULONG pct;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Flags);

    Cpu = KeGetCurrentProcessorIndex();
    if (Cpu >= AMDPPM_MAX_PROCESSORS || !Frequency || !Selection)
        return TargetPercent;

    DevExt = AmdPpmPerfFdoByProcessor[Cpu];
    if (!DevExt || !DevExt->PSS || DevExt->PSS->Count == 0)
    {
        *Frequency = 0;
        *Selection = 0;
        return TargetPercent;
    }

    nominal = DevExt->PSS->States[0].CoreFrequency;
    if (nominal == 0)
    {
        *Frequency = 0;
        *Selection = 0;
        return TargetPercent;
    }

    maxIdx = DevExt->PSS->Count - 1;
    slowest = maxIdx;
    if ((DevExt->PPMEnabled & AMD_CAP_PPC) && DevExt->PSS)
        slowest = AmdPpmMinUlong(maxIdx, DevExt->PPC_Cap);

    t = TargetPercent;
    if (t < MinPercent)
        t = MinPercent;
    if (t > MaxPercent)
        t = MaxPercent;

    /* 100 % = P0 (index 0); 0 % = slowest allowed P-state */
    idx = ((100 - t) * slowest) / 100;
    if (idx > slowest)
        idx = slowest;

    *Frequency = DevExt->PSS->States[idx].CoreFrequency;
    *Selection = idx;
    pct = (*Frequency * 100) / nominal;
    if (pct > 100)
        pct = 100;
    return pct;
}

VOID
FASTCALL
AmdPpmPerfControl(
    _In_ ULONG Context,
    _In_ ULONGLONG SelectedState,
    _In_ ULONG MinPercent,
    _In_ ULONG MaxPercent,
    _In_ ULONG TolerancePercent,
    _In_ UCHAR Autonomous,
    _In_ UCHAR Initiate,
    _In_ UCHAR Force)
{
    PFDO_DATA DevExt;
    ULONG Cpu;
    ULONG idx;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(MinPercent);
    UNREFERENCED_PARAMETER(MaxPercent);
    UNREFERENCED_PARAMETER(TolerancePercent);
    UNREFERENCED_PARAMETER(Autonomous);

    if (!Initiate && !Force)
        return;

    Cpu = KeGetCurrentProcessorIndex();
    if (Cpu >= AMDPPM_MAX_PROCESSORS)
        return;

    DevExt = AmdPpmPerfFdoByProcessor[Cpu];
    if (!DevExt || !DevExt->PSS || DevExt->PSS->Count == 0)
        return;

    idx = (ULONG)SelectedState;
    if (idx >= DevExt->PSS->Count)
        idx = DevExt->PSS->Count - 1;

    if (DevExt->DrvCapabilities & AMD_CAP_FFH)
    {
        Status = SetFFHPState((ULONGLONG)idx, (ULONGLONG)idx);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("AmdPpmPerfControl: SetFFHPState failed 0x%lx idx=%lu\n",
                    Status, idx);
        }
    }
    else if (DevExt->SetPState)
    {
        Status = DevExt->SetPState(0, (ULONGLONG)idx, (ULONGLONG)idx);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("AmdPpmPerfControl: SetPState failed 0x%lx idx=%lu\n",
                    Status, idx);
        }
    }
}

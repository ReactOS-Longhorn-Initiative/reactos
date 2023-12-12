/*
 * PROJECT:     ReactOS Kernel - Vista+ APIs
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Ke functions of Vista+
 * COPYRIGHT:   2016 Pierre Schweitzer (pierre@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>

NTKRNLVISTAAPI
ULONG
NTAPI
KeQueryActiveProcessorCount(OUT PKAFFINITY ActiveProcessors OPTIONAL)
{
    RTL_BITMAP Bitmap;
    KAFFINITY ActiveMap = KeQueryActiveProcessors();

    if (ActiveProcessors != NULL)
    {
        *ActiveProcessors = ActiveMap;
    }

    RtlInitializeBitMap(&Bitmap, (PULONG)&ActiveMap,  sizeof(ActiveMap) * 8);
    return RtlNumberOfSetBits(&Bitmap);
}

NTKRNLVISTAAPI
ULONG
NTAPI
KeQueryMaximumProcessorCountEx(_In_ USHORT GroupNumber)
{
    UNIMPLEMENTED;
	return 1;
}

NTKRNLVISTAAPI
ULONG
NTAPI
KeQueryActiveProcessorCountEx(_In_ USHORT GroupNumber)
{
    UNIMPLEMENTED;
	return 1;
}

NTKRNLVISTAAPI
USHORT
NTAPI
KeQueryHighestNodeNumber()
{
    UNIMPLEMENTED;
	return 0;
}

NTKRNLVISTAAPI
USHORT
NTAPI
KeGetCurrentNodeNumber()
{
    UNIMPLEMENTED_ONCE;
	return 0;
}

NTKRNLVISTAAPI
ULONG
NTAPI
KeGetCurrentProcessorNumberEx(
  _Out_opt_ PPROCESSOR_NUMBER ProcNumber
)
{
    if (ProcNumber != NULL)
    {
        ProcNumber->Group=0;
        ProcNumber->Number=0;
        ProcNumber->Reserved=0;
    }

    UNIMPLEMENTED_ONCE;
    return 0;
}


NTKRNLVISTAAPI
USHORT
NTAPI
KeQueryActiveGroupCount(VOID)
{
    UNIMPLEMENTED;
    return 1;
}

NTKRNLVISTAAPI
KAFFINITY
NTAPI
KeQueryGroupAffinity(
  _In_ USHORT GroupNumber)
{
    UNIMPLEMENTED;
    return 0;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
NTSTATUS
NTAPI
KeQueryLogicalProcessorRelationship(
    _In_opt_ PPROCESSOR_NUMBER ProcessorNumber,
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_writes_bytes_opt_(*Length) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Information,
    _Inout_ PULONG Length)
{
    UNIMPLEMENTED;
    return STATUS_INVALID_PARAMETER;
}

NTKRNLVISTAAPI
VOID
NTAPI
KeQueryNodeActiveAffinity(
    _In_ USHORT NodeNumber,
    _Out_opt_ PGROUP_AFFINITY Affinity,
    _Out_opt_ PUSHORT Count)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
ULONGLONG
NTAPI
KeQueryUnbiasedInterruptTime(VOID)
{
    UNIMPLEMENTED;
    return 1;
}

NTKRNLVISTAAPI
NTSTATUS
NTAPI
KeQueryDpcWatchdogInformation(
  OUT PKDPC_WATCHDOG_INFORMATION WatchdogInformation
)
{
    WatchdogInformation->DpcTimeLimit = 0;
    WatchdogInformation->DpcTimeCount = 0;
    WatchdogInformation->DpcWatchdogLimit = 0;
    WatchdogInformation->DpcWatchdogCount = 0;
    WatchdogInformation->Reserved = 0;

    UNIMPLEMENTED_ONCE;
    return STATUS_UNSUCCESSFUL;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
BOOLEAN
NTAPI
KeSetCoalescableTimer(
    _Inout_ PKTIMER Timer,
    _In_ LARGE_INTEGER DueTime,
    _In_ ULONG Period,
    _In_ ULONG TolerableDelay,
    _In_opt_ PKDPC Dpc)
{
    return KeSetTimerEx(Timer, DueTime, Period, Dpc);
}

NTKRNLVISTAAPI
NTSTATUS
NTAPI
KeSetTargetProcessorDpcEx(
    _Inout_ PKDPC Dpc,
    _In_ PPROCESSOR_NUMBER ProcNumber)
{

    /* TODO: Check if ProcNumber is vaild */

    KeSetTargetProcessorDpc(Dpc, ProcNumber->Number);
    return STATUS_SUCCESS;
}

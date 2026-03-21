/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Portable processor related routines
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

KAFFINITY KeActiveProcessors = 0;

/* Number of processors */
CCHAR KeNumberProcessors = 0;

#ifdef CONFIG_SMP

/* Theoretical maximum number of processors that can be handled.
 * Set once at run-time. Returned by KeQueryMaximumProcessorCount(). */
ULONG KeMaximumProcessors = MAXIMUM_PROCESSORS;

/* Maximum number of logical processors that can be started
 * (including dynamically) at run-time. If 0: do not perform checks. */
ULONG KeNumprocSpecified = 0;

/* Maximum number of logical processors that can be started
 * at boot-time. If 0: do not perform checks. */
ULONG KeBootprocSpecified = 0;

#endif // CONFIG_SMP

/* FUNCTIONS *****************************************************************/

KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID)
{
    return KeActiveProcessors;
}

/*
 * @implemented
 * Logical processor index in the single-group configuration (matches
 * KeGetCurrentProcessorNumber on legacy x86/x64).
 */
ULONG
NTAPI
KeGetCurrentProcessorIndex(VOID)
{
    return (ULONG)KeGetCurrentProcessorNumber();
}

/*
 * @implemented
 */
ULONG
NTAPI
KeQueryMaximumProcessorCount(VOID)
{
#ifdef CONFIG_SMP
    return KeMaximumProcessors;
#else
    return 1;
#endif
}

/*
 * @implemented
 * ReactOS uses a single processor group (0). ALL_PROCESSOR_GROUPS returns
 * the total active logical processor count.
 */
ULONG
NTAPI
KeQueryActiveProcessorCountEx(
    _In_ USHORT GroupNumber)
{
    if (GroupNumber != 0 && GroupNumber != (USHORT)0xFFFF) /* ALL_PROCESSOR_GROUPS */
        return 0;

    return (ULONG)KeNumberProcessors;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeGetProcessorNumberFromIndex(
    _In_ ULONG ProcIndex,
    _Out_ PPROCESSOR_NUMBER ProcNumber)
{
    if (ProcIndex >= (ULONG)KeNumberProcessors)
        return STATUS_INVALID_PARAMETER;

    ProcNumber->Group = 0;
    ProcNumber->Number = (UCHAR)ProcIndex;
    ProcNumber->Reserved = 0;
    return STATUS_SUCCESS;
}

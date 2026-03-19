/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor Power Management performance handling
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * DPC routine that fires periodically to sample processor performance state
 * and adjust throttling accordingly.
 *
 * @param[in] Dpc
 * The DPC object associated with this performance timer.
 *
 * @param[in] DeferredContext
 * A pointer to the PKPRCB (Processor Control Block) for the target processor.
 *
 * @param[in] SystemArgument1
 * Unused.
 *
 * @param[in] SystemArgument2
 * Unused.
 *
 * @remarks
 * A full implementation would evaluate the current processor idle time vs.
 * busy time, look up the appropriate P-state in the processor's performance
 * state table, and request a frequency/voltage transition if needed. This
 * requires HAL/ACPI _PSS support which is not yet available. For now the
 * routine only updates the performance accounting timestamps so that the
 * system does not accumulate stale data.
 */
VOID
NTAPI
PpmPerfIdleDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    PKPRCB Prcb = (PKPRCB)DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (Prcb == NULL)
    {
        return;
    }

    /*
     * Record the current performance timer tick. A complete performance
     * governor would compare KernelTime, UserTime and IdleTime over successive
     * samples to determine utilization, then walk the _PSS P-state table and
     * issue an ACPI _PPC/_PCT write to adjust the processor frequency/voltage.
     *
     * FIXME: Implement P-state evaluation and transition once HAL exposes
     * processor performance controls via HalSetSystemInformation.
     */
    Prcb->PowerState.LastSysTime = KeQueryInterruptTime();
}

/* EOF */

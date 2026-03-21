/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor Power Management C/P-state statistics and accounting
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 *
 * @file        ntoskrnl/po/ppm/cpustat.c
 *
 * @brief
 * Maintains per-processor power-state accounting data.  Three classes of
 * statistics are tracked:
 *
 *  1. Idle-state (C-state) residency – how much time the processor spent
 *     in each C-state and how many transitions occurred.
 *
 *  2. Busy-percentage – a 0-100 utilisation estimate derived from the
 *     difference in (KernelTime + UserTime) between successive DPC samples.
 *     The value drives P-state promotion and demotion in policy.c.
 *
 *  3. Reset helpers – bring accounting fields back to a clean state when
 *     the power policy changes or the processor driver reloads.
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Records that the processor spent @p Duration 100-ns units in C-state
 * @p StateIndex.
 *
 * @param[in,out] PowerState
 * Per-processor power state block (Prcb->PowerState).
 *
 * @param[in] StateIndex
 * Index into the C-state table (0 = C1, 1 = C2, 2 = C3).
 * Indices beyond the tracked range are clamped to the deepest bucket.
 *
 * @param[in] Duration
 * Time spent in the state, in 100-ns interrupt-time units.
 *
 * @remarks
 * Called from PpmIdle after each idle period completes (i.e., after the
 * processor wakes from a C-state entry).  Must be IRQL-neutral since it
 * only updates non-atomic ULONGLONG accumulators that are written only by
 * the owning processor.
 */
VOID
NTAPI
PpmUpdateIdleAccounting(
    _In_ PPROCESSOR_POWER_STATE PowerState,
    _In_ ULONG                  StateIndex,
    _In_ ULONGLONG              Duration)
{
    /* Clamp to the last tracked bucket (C1 / C2 / C3) */
    if (StateIndex >= RTL_NUMBER_OF(PowerState->TotalIdleStateTime))
        StateIndex = RTL_NUMBER_OF(PowerState->TotalIdleStateTime) - 1;

    PowerState->TotalIdleStateTime[StateIndex]    += Duration;
    PowerState->TotalIdleTransitions[StateIndex]++;

    PPMTRACE(PPM_PERF_DEBUG,
             "PpmUpdateIdleAccounting: CPU %u C%u += %I64u us "
             "(total=%I64u, transitions=%lu)\n",
             KeGetCurrentProcessorNumber(),
             StateIndex + 1,
             Duration / 10,
             PowerState->TotalIdleStateTime[StateIndex] / 10,
             PowerState->TotalIdleTransitions[StateIndex]);
}

/**
 * @brief
 * Computes the current processor busy percentage from accumulated kernel
 * and user time since the last DPC sample period.
 *
 * @param[in] Prcb
 * Processor Control Block of the target processor.
 *
 * @return
 * A value in the range [0, 100] representing the percentage of time the
 * processor spent executing non-idle work since the last call.
 *
 * @remarks
 * Called once per PerfDpc period (20 ms by default) by PpmPerfIdleDpcRoutine.
 * The result is stored in @c Prcb->PowerState.LastBusyPercentage and
 * subsequently consumed by PpmEvaluatePerfPolicy to drive P-state selection.
 *
 * The algorithm:
 *   elapsed_busy  = (KernelTime + UserTime) - LastKernelUserTime
 *   busy_percent  = min(elapsed_busy, DPC_PERIOD_TICKS) * 100
 *                   / DPC_PERIOD_TICKS
 *
 * KernelTime/UserTime are in clock ticks (10 ms each at 100 Hz).  The
 * DPC fires every PPM_PERF_DPC_PERIOD_MS, so we expect at most 2 ticks
 * per period at the default 100-Hz clock.  The ratio is still meaningful
 * because it captures whether the processor was active at all during the
 * sampling window.
 */
UCHAR
NTAPI
PpmComputeBusyPercentage(
    _In_ PKPRCB Prcb)
{
    PPROCESSOR_POWER_STATE PowerState = &Prcb->PowerState;
    ULONG CurrentKernelUserTime;
    ULONG ElapsedBusy;
    ULONG SampleWindow;
    UCHAR BusyPercent;

    /*
     * Snapshot the current (KernelTime + UserTime) in clock ticks.
     * Both fields are ULONG (wraps at ~497 days at 100 Hz) and updated
     * by the clock interrupt handler, so we may observe a race; the
     * error is at most one tick (10 ms) which is acceptable for power
     * management purposes.
     */
    CurrentKernelUserTime = Prcb->KernelTime + Prcb->UserTime;

    ElapsedBusy = CurrentKernelUserTime - PowerState->LastKernelUserTime;
    PowerState->LastKernelUserTime = CurrentKernelUserTime;

    /*
     * The DPC period expressed in the same 10-ms clock-tick units.
     * PPM_PERF_DPC_PERIOD_MS / 10 = 2 ticks at the default 20-ms period.
     * Use a minimum of 1 to avoid division by zero on very fast CPUs or
     * pathological timer resolutions.
     */
    SampleWindow = max(PPM_PERF_DPC_PERIOD_MS / 10, 1);

    /* Clamp elapsed to the window so we never exceed 100 % */
    if (ElapsedBusy > SampleWindow)
        ElapsedBusy = SampleWindow;

    BusyPercent = (UCHAR)((ElapsedBusy * 100) / SampleWindow);

    PowerState->LastBusyPercentage = BusyPercent;

    PPMTRACE(PPM_PERF_DEBUG,
             "PpmComputeBusyPercentage: CPU %u busy=%u%%\n",
             Prcb->Number, BusyPercent);

    return BusyPercent;
}

/**
 * @brief
 * Resets all idle-state accounting fields in @p PowerState to zero.
 *
 * @param[in,out] PowerState
 * Per-processor power state block to reset.
 *
 * @remarks
 * Called when power policy changes substantially (e.g. AC/DC transition)
 * so that stale statistics do not influence the new policy evaluation.
 */
VOID
NTAPI
PpmResetIdleAccounting(
    _In_ PPROCESSOR_POWER_STATE PowerState)
{
    RtlZeroMemory(PowerState->TotalIdleStateTime,
                  sizeof(PowerState->TotalIdleStateTime));
    RtlZeroMemory(PowerState->TotalIdleTransitions,
                  sizeof(PowerState->TotalIdleTransitions));

    PowerState->LastBusyPercentage         = 0;
    PowerState->LastAdjustedBusyPercentage = 0;
    PowerState->LastC3Percentage           = 0;
    PowerState->PreviousC3StateTime        = 0;

    PPMTRACE(PPM_PERF_DEBUG,
             "PpmResetIdleAccounting: CPU %u statistics cleared\n",
             KeGetCurrentProcessorNumber());
}

/**
 * @brief
 * Clears C-state accounting and busy-percent snapshots on every logical
 * processor.  Invoked when the active system power policy changes so idle
 * selection does not use stale utilisation after AC/DC or policy updates.
 */
VOID
NTAPI
PpmResetIdleAccountingAllProcessors(VOID)
{
    ULONG i;

    for (i = 0; i < (ULONG)KeNumberProcessors; i++)
    {
        PKPRCB Prcb = KiProcessorBlock[i];
        if (Prcb != NULL)
            PpmResetIdleAccounting(&Prcb->PowerState);
    }
}

/* EOF */

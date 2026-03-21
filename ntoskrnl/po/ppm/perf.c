/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor Power Management – performance state DPC
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 *              Copyright 2025 ReactOS Contributors
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <internal/ppm.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * DPC routine fired periodically on each processor to evaluate the
 * processor performance (P-state) policy.
 *
 * @param[in] Dpc
 * The DPC object associated with this performance timer (embedded in
 * Prcb->PowerState.PerfDpc).
 *
 * @param[in] DeferredContext
 * Pointer to the KPRCB of the target processor.  This is the PRCB on which
 * the DPC runs (set via KeSetTargetProcessorDpc in PpmInitialize).
 *
 * @param[in] SystemArgument1
 * Unused.
 *
 * @param[in] SystemArgument2
 * Unused.
 *
 * @remarks
 * The DPC runs at DISPATCH_LEVEL on the owning processor.  It:
 *
 *  1. Increments the DPC tick counter for diagnostic purposes.
 *
 *  2. Records the current interrupt time in PowerState->LastSysTime so that
 *     the idle path has an up-to-date reference for duration estimation.
 *
 *  3. Delegates the actual policy evaluation to PpmEvaluatePerfPolicy
 *     (policy.c) which computes the busy percentage (cpustat.c) and
 *     selects the appropriate P-state (eng.c).
 *
 *  4. Re-arms the periodic timer for the next sample period.
 *
 * The DPC and timer are created and initialised during the late phase of
 * PpmInitialize().  The timer is armed there with KeSetTimerEx using
 * PPM_PERF_DPC_PERIOD (200 000 × 100 ns = 20 ms) as the recurrence period.
 */
VOID
NTAPI
PpmPerfIdleDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    PKPRCB                 Prcb = (PKPRCB)DeferredContext;
    PPROCESSOR_POWER_STATE PowerState;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (Prcb == NULL)
        return;

    PowerState = &Prcb->PowerState;

    /* ------------------------------------------------------------------- */
    /* Step 1: Update diagnostic tick counter                               */
    /* ------------------------------------------------------------------- */
    PowerState->PerfTickCount++;

    /* ------------------------------------------------------------------- */
    /* Step 2: Snapshot the current interrupt time                          */
    /*                                                                       */
    /* PpmIdle uses LastSysTime as a reference when estimating the upcoming  */
    /* idle duration (via _PpmEstimateIdleDuration in idle.c).               */
    /* ------------------------------------------------------------------- */
    PowerState->LastSysTime = (ULONG)KeQueryInterruptTime();

    /* ------------------------------------------------------------------- */
    /* Step 3: Policy evaluation                                             */
    /*                                                                       */
    /* PpmEvaluatePerfPolicy samples utilisation, selects the optimal        */
    /* P-state, applies hysteresis, and calls PpmApplyThrottle if needed.    */
    /* ------------------------------------------------------------------- */
    PpmEvaluatePerfPolicy(Prcb);

#if defined(CONFIG_SMP)
    if (Prcb->Number == 0 && KeNumberProcessors > 1)
        PpmCoreParkingPeriodicRebalance();
#endif

    PPMTRACE(PPM_PERF_DEBUG,
             "PpmPerfIdleDpcRoutine: CPU %u tick=%lu throttle=%u%%\n",
             Prcb->Number,
             PowerState->PerfTickCount,
             PowerState->CurrentThrottle);
}

/* EOF */

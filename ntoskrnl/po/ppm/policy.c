/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor Power Management – performance policy evaluation
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 *
 * @file        ntoskrnl/po/ppm/policy.c
 *
 * @brief
 * Implements the top-level processor performance policy engine.
 *
 * PpmEvaluatePerfPolicy() is the central entry point.  It is called once per
 * DPC period (PPM_PERF_DPC_PERIOD_MS, default 20 ms) by
 * PpmPerfIdleDpcRoutine on the owning processor and performs the following
 * steps:
 *
 *  1. Compute the current busy percentage (cpustat.c → PpmComputeBusyPercentage).
 *  2. Select the optimal throttle level (eng.c → PpmSelectPerfState).
 *  3. Apply hysteresis to avoid high-frequency oscillation (promotion /
 *     demotion counters).
 *  4. Commit the new throttle level if it differs from the current one
 *     (eng.c → PpmApplyThrottle).
 *
 * Hysteresis model
 * ────────────────
 * Windows uses per-state IncreaseCount / DecreaseCount to suppress rapid
 * oscillation.  Our simplified model keeps two saturating counters
 * (PromotionCount, DemotionCount) already present in PROCESSOR_POWER_STATE:
 *
 *  • If the engine wants to raise throttle:
 *      – Increment PromotionCount, clear DemotionCount.
 *      – Promote only when PromotionCount reaches PPM_POLICY_PROMOTE_THRESHOLD.
 *
 *  • If the engine wants to lower throttle:
 *      – Increment DemotionCount, clear PromotionCount.
 *      – Demote only when DemotionCount reaches PPM_POLICY_DEMOTE_THRESHOLD.
 *
 *  • If the target matches current: reset both counters.
 *
 * Using asymmetric thresholds (promote fast, demote slowly) mirrors the
 * Windows "performance" profile behaviour and avoids the "CPU stuck at
 * minimum when just one high-load burst occurs" failure mode.
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/*
 * PPM_POLICY_PROMOTE_THRESHOLD and PPM_POLICY_DEMOTE_THRESHOLD are no longer
 * compile-time constants.  They are read at run-time from the global variables
 * PpmPolicyPerfIncreaseTime and PpmPolicyPerfDecreaseTime, which are updated by
 * the GUID_PROCESSOR_PERF_INCREASE_TIME and GUID_PROCESSOR_PERF_DECREASE_TIME
 * power setting workers.  Default values (1 and 5 respectively) are set in
 * dispatch.c at driver initialisation time.
 */

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Top-level processor performance policy evaluator.
 *
 * @param[in] Prcb
 * Processor Control Block of the currently running processor.
 *
 * @remarks
 * Must be called at DISPATCH_LEVEL from the per-processor performance DPC
 * (PpmPerfIdleDpcRoutine).  It touches only the DPC-owning processor's
 * PRCB, so no cross-processor synchronisation is needed.
 *
 * The function is a no-op when:
 *   • ProcessorMaxThrottle == ProcessorMinThrottle (fixed-frequency system), or
 *   • No performance driver has registered P-states (IdleHandlers == NULL).
 */
VOID
NTAPI
PpmEvaluatePerfPolicy(
    _In_ PKPRCB Prcb)
{
    PPROCESSOR_POWER_STATE PowerState = &Prcb->PowerState;
    UCHAR BusyPercentage;
    UCHAR TargetThrottle;
    UCHAR CurrentThrottle;

    /* Quick exit: nothing to do if the processor runs at a fixed frequency */
    if (PowerState->ProcessorMaxThrottle == PowerState->ProcessorMinThrottle)
        return;

    /* No P-state driver registered; nothing to evaluate */
    if (PowerState->IdleHandlers == NULL)
        return;

    /* ------------------------------------------------------------------- */
    /* Step 1: Sample processor utilisation                                  */
    /* ------------------------------------------------------------------- */
    BusyPercentage = PpmComputeBusyPercentage(Prcb);

    /* ------------------------------------------------------------------- */
    /* Step 2: Determine the optimal throttle level for this utilisation     */
    /* ------------------------------------------------------------------- */
    TargetThrottle  = PpmSelectPerfState(Prcb, BusyPercentage);
    CurrentThrottle = PowerState->CurrentThrottle;

    /* ------------------------------------------------------------------- */
    /* Step 3: Apply hysteresis                                              */
    /* ------------------------------------------------------------------- */
    if (TargetThrottle > CurrentThrottle)
    {
        /*
         * Load is increasing – promote (raise clock frequency).
         * Use a low threshold so the CPU reacts quickly to load spikes.
         */
        PowerState->PromotionCount++;
        PowerState->DemotionCount = 0;

        if (PowerState->PromotionCount >= PpmPolicyPerfIncreaseTime)
        {
            PowerState->PromotionCount = 0;

            PPMTRACE(PPM_PERF_DEBUG,
                     "PpmEvaluatePerfPolicy: CPU %u PROMOTE %u%% → %u%%"
                     " (busy=%u%%)\n",
                     Prcb->Number, CurrentThrottle, TargetThrottle,
                     BusyPercentage);

            PpmApplyThrottle(PowerState, TargetThrottle);
        }
    }
    else if (TargetThrottle < CurrentThrottle)
    {
        /*
         * Load has decreased – demote (lower clock frequency).
         * Use a higher threshold to avoid oscillation on bursty workloads.
         */
        PowerState->DemotionCount++;
        PowerState->PromotionCount = 0;

        if (PowerState->DemotionCount >= PpmPolicyPerfDecreaseTime)
        {
            PowerState->DemotionCount = 0;

            PPMTRACE(PPM_PERF_DEBUG,
                     "PpmEvaluatePerfPolicy: CPU %u DEMOTE %u%% → %u%%"
                     " (busy=%u%%)\n",
                     Prcb->Number, CurrentThrottle, TargetThrottle,
                     BusyPercentage);

            PpmApplyThrottle(PowerState, TargetThrottle);
        }
    }
    else
    {
        /* Throttle is optimal – reset both counters */
        PowerState->PromotionCount = 0;
        PowerState->DemotionCount  = 0;
    }
}

/* EOF */

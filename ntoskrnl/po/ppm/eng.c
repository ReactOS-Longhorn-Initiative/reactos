/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor Power Management core engine
 * COPYRIGHT:   Copyright 2025 ReactOS Contributors
 *
 * @file        ntoskrnl/po/ppm/eng.c
 *
 * @brief
 * Implements the low-level selection and transition helpers that are shared
 * between the idle path (PpmIdle → PpmSelectIdleState) and the periodic
 * performance DPC path (PpmPerfIdleDpcRoutine → PpmEvaluatePerfPolicy →
 * PpmSelectPerfState / PpmApplyThrottle).
 *
 * C-state selection (PpmSelectIdleState)
 * ──────────────────────────────────────
 * Uses the variable-length State[] array (PPM_PROCESSOR_IDLE_STATE_EX) that
 * follows PROCESSOR_IDLE_STATES_EX in memory: deepest-first search against
 * estimated idle duration (2×Latency and BreakEven in µs), driver IdleTest /
 * IdleAvailabilityCheck, then GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD /
 * _PROMOTE_THRESHOLD via LastBusyPercentage.  Without a registered table,
 * returns MAXULONG so the caller uses HalProcessorIdle.
 *
 * P-state selection (PpmSelectPerfState)
 * ──────────────────────────────────────
 * Given the current busy percentage (computed by cpustat.c), compare it
 * against the policy limits stored in Prcb->PowerState:
 *   • ProcessorMaxThrottle – upper ceiling (from platform / thermal caps)
 *   • ProcessorMinThrottle – lower floor
 *   • CurrentThrottle      – the last committed throttle level
 *
 * The function returns the new target throttle percentage.  An increase
 * (promotion) is triggered immediately; a decrease (demotion) is gradual
 * to avoid oscillation.
 *
 * Throttle application (PpmApplyThrottle)
 * ────────────────────────────────────────
 * Commits throttle via legacy PerfSetThrottle, Windows-style
 * PROCESSOR_PERF_STATES PerfSelectionHandler + PerfControlHandler, or
 * HalSetSystemInformation(HalProcessorSpeedInformation) as a last resort.
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE HELPERS ************************************************************/

/*
 * _PpmIdleStates
 *
 * Returns the PPM_IDLE_STATES_EX pointer stored in PowerState->IdleState,
 * or NULL if no driver has registered idle states for this processor.
 */
static FORCEINLINE
PPPM_IDLE_STATES_EX
_PpmIdleStates(
    _In_ PPROCESSOR_POWER_STATE PowerState)
{
    return (PPPM_IDLE_STATES_EX)PowerState->IdleState;
}

/*
 * _PpmIdleStateRef
 *
 * Returns a pointer to C-state entry @p Index in the variable-length array
 * that immediately follows the PROCESSOR_IDLE_STATES_EX header in memory.
 */
static FORCEINLINE
PPPM_PROCESSOR_IDLE_STATE_EX
_PpmIdleStateRef(
    _In_ PPPM_IDLE_STATES_EX IdleStates,
    _In_ ULONG Index)
{
    return (PPPM_PROCESSOR_IDLE_STATE_EX)
        ((PUCHAR)IdleStates + sizeof(PPM_IDLE_STATES_EX) +
         (SIZE_T)Index * sizeof(PPM_PROCESSOR_IDLE_STATE_EX));
}

/*
 * _PpmIdleSelectByBudget
 *
 * Walk from deepest to shallowest and pick the first state whose transition
 * cost fits in @p IdleDuration100Ns.  Uses 2×Latency (µs) as round-trip
 * budget and optional BreakEvenDuration (µs) when non-zero.
 */
static ULONG
_PpmIdleSelectByBudget(
    _In_ PPPM_IDLE_STATES_EX IdleStates,
    _In_ ULONGLONG IdleDuration100Ns)
{
    PVOID Context = IdleStates->Context;
    ULONG StateCount = IdleStates->ProcessorIdleCount;
    LONG i;

    for (i = (LONG)StateCount - 1; i >= 0; i--)
    {
        ULONG idx = (ULONG)i;
        PPPM_PROCESSOR_IDLE_STATE_EX S = _PpmIdleStateRef(IdleStates, idx);
        ULONG latUs = S->Latency ? S->Latency : 1;
        ULONGLONG roundTrip100Ns = (ULONGLONG)latUs * 20ULL; /* 2 × µs → 100 ns */

        if (IdleDuration100Ns < roundTrip100Ns)
            continue;

        if (S->BreakEvenDuration != 0)
        {
            ULONGLONG breakEven100Ns = (ULONGLONG)S->BreakEvenDuration * 10ULL;
            if (IdleDuration100Ns < breakEven100Ns)
                continue;
        }

        if (IdleStates->IdleTest != NULL)
        {
            ULONG durUs = (ULONG)(IdleDuration100Ns / 10ULL);
            if (durUs == 0 && IdleDuration100Ns != 0)
                durUs = 1;
            if (IdleStates->IdleTest(Context, idx, durUs) == 0)
                continue;
        }

        if (IdleStates->IdleAvailabilityCheck != NULL &&
            IdleStates->IdleAvailabilityCheck(Context, idx) == 0)
            continue;

        return idx;
    }

    return 0;
}

/*
 * _PpmIdleApplyPromoteDemote
 *
 * Applies GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD / _PROMOTE_THRESHOLD using the
 * last sampled busy percentage (DPC path): shallow C-states when the CPU is
 * rarely idle, deeper states only when recent idle fraction exceeds promote.
 */
static ULONG
_PpmIdleApplyPromoteDemote(
    _In_ PPROCESSOR_POWER_STATE PowerState,
    _In_ PPPM_IDLE_STATES_EX IdleStates,
    _In_ ULONG Selected)
{
    ULONG idlePct;
    ULONG demote;
    ULONG promote;
    ULONG maxIndex;

    idlePct = 100UL - (ULONG)PowerState->LastBusyPercentage;
    if (idlePct > 100UL)
        idlePct = 100UL;

    demote = (ULONG)(UCHAR)PpmPolicyIdleDemoteThreshold;
    promote = (ULONG)(UCHAR)PpmPolicyIdlePromoteThreshold;

    if (promote < demote)
    {
        ULONG t = promote;
        promote = demote;
        demote = t;
    }

    maxIndex = IdleStates->ProcessorIdleCount;
    if (maxIndex == 0)
        return Selected;

    if (idlePct < demote)
        return 0;

    if (idlePct < promote && maxIndex > 2 && Selected > 1)
        Selected = 1;

    if (Selected >= maxIndex)
        Selected = maxIndex - 1;

    return Selected;
}

/*
 * _PpmIdleBackoffIfUnavailable
 *
 * If the chosen state is vetoed, walk toward C1 (index 0).
 */
static ULONG
_PpmIdleBackoffIfUnavailable(
    _In_ PPPM_IDLE_STATES_EX IdleStates,
    _In_ ULONG Selected)
{
    PVOID Context = IdleStates->Context;

    while (Selected > 0 &&
           IdleStates->IdleAvailabilityCheck != NULL &&
           IdleStates->IdleAvailabilityCheck(Context, Selected) == 0)
    {
        Selected--;
    }

    return Selected;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Selects the most energy-efficient C-state the processor should enter for
 * an idle period of approximately @p IdleDuration 100-ns units.
 *
 * @param[in] PowerState
 * Per-processor power state block (Prcb->PowerState).
 *
 * @param[in] IdleDuration
 * Estimated idle duration in 100-ns interrupt-time units.
 * Pass 0 if the duration is unknown; the driver’s IdlePreselect is used
 * when present, otherwise state 0.
 *
 * @return
 * Zero-based index into the driver-provided C-state table.
 * 0 always means "shallowest available state" (C1 / HLT equivalent).
 * MAXULONG is returned when no driver table is available (caller should
 * use HalProcessorIdle instead).
 *
 * @remarks
 * When @p IdleDuration is non-zero, walks the @c State[] tail of
 * @c PROCESSOR_IDLE_STATES_EX (see @c PPM_PROCESSOR_IDLE_STATE_EX) from
 * deepest to shallowest and picks the first state that fits the latency /
 * break-even budget and passes @c IdleTest / @c IdleAvailabilityCheck.
 *
 * @c PpmPolicyIdleDemoteThreshold and @c PpmPolicyIdlePromoteThreshold
 * then bias toward shallow or mid C-states based on recent idle fraction
 * (@c LastBusyPercentage from the perf DPC).
 */
ULONG
NTAPI
PpmSelectIdleState(
    _In_ PPROCESSOR_POWER_STATE PowerState,
    _In_ ULONGLONG              IdleDuration)
{
    PPPM_IDLE_STATES_EX IdleStates;
    PVOID               Context;
    ULONG               StateCount;
    ULONG               Selected;

    IdleStates = _PpmIdleStates(PowerState);

    if (IdleStates == NULL || IdleStates->ProcessorIdleCount == 0)
        return MAXULONG;

    Context = IdleStates->Context;
    StateCount = IdleStates->ProcessorIdleCount;

    if (IdleDuration == 0)
    {
        if (IdleStates->IdlePreselect != NULL)
            Selected = IdleStates->IdlePreselect(Context, NULL);
        else
            Selected = 0;

        if (Selected >= StateCount)
            Selected = StateCount - 1;
    }
    else
    {
        Selected = _PpmIdleSelectByBudget(IdleStates, IdleDuration);
    }

    Selected = _PpmIdleApplyPromoteDemote(PowerState, IdleStates, Selected);
    Selected = _PpmIdleBackoffIfUnavailable(IdleStates, Selected);

    if (Selected >= StateCount)
        Selected = StateCount - 1;

    return Selected;
}

/**
 * @brief
 * Selects the target throttle percentage for the current processor based on
 * the observed busy percentage @p BusyPercentage.
 *
 * @param[in] Prcb
 * Processor Control Block of the target processor.
 *
 * @param[in] BusyPercentage
 * Processor utilisation in the range [0, 100], as returned by
 * PpmComputeBusyPercentage().
 *
 * @return
 * Target throttle percentage in [ProcessorMinThrottle, ProcessorMaxThrottle].
 *
 * @remarks
 * The algorithm uses a threshold-based demand governor:
 *
 *   • If BusyPercentage >= PpmPolicyPerfIncreaseThreshold (default 60 %):
 *       target = ProcessorMaxThrottle  (snap to maximum performance)
 *
 *   • If BusyPercentage <= PpmPolicyPerfDecreaseThreshold (default 40 %):
 *       target = ProcessorMinThrottle  (snap to minimum performance)
 *
 *   • Otherwise (BusyPercentage is in the neutral band):
 *       target is linearly interpolated between MinThrottle and MaxThrottle
 *       proportional to where BusyPercentage sits within the band.
 *
 * This mirrors the Windows "Balanced" profile policy: quick to ramp up,
 * gradual to ramp down (the actual timing of the transition is controlled by
 * PpmPolicyPerfIncreaseTime / PpmPolicyPerfDecreaseTime in policy.c).
 *
 * The thresholds and throttle limits are configurable at run-time via the
 * power setting workers (posett.c) that service GUID_PROCESSOR_PERF_*
 * and GUID_PROCESSOR_THROTTLE_* settings.
 *
 * The result is clamped to [ProcessorMinThrottle, ProcessorMaxThrottle]
 * to respect platform/thermal caps registered via RegisterPerfCap.
 */
UCHAR
NTAPI
PpmSelectPerfState(
    _In_ PKPRCB Prcb,
    _In_ UCHAR  BusyPercentage)
{
    PPROCESSOR_POWER_STATE PowerState = &Prcb->PowerState;
    UCHAR MaxThrottle;
    UCHAR MinThrottle;
    UCHAR IncreaseThresh;
    UCHAR DecreaseThresh;
    ULONG BandRange;
    UCHAR Target;

    MaxThrottle = PowerState->ProcessorMaxThrottle;
    MinThrottle = PowerState->ProcessorMinThrottle;

    /* Clamp to valid range to guard against misconfigured drivers */
    if (MaxThrottle > 100)
        MaxThrottle = 100;
    if (MinThrottle > MaxThrottle)
        MinThrottle = MaxThrottle;

    /*
     * Snapshot the global thresholds.  These are updated from a different
     * thread (the power setting worker), so read them once to keep the
     * decision consistent.
     */
    IncreaseThresh = PpmPolicyPerfIncreaseThreshold;
    DecreaseThresh = PpmPolicyPerfDecreaseThreshold;

    /* Keep the thresholds sane relative to each other */
    if (DecreaseThresh >= IncreaseThresh)
        DecreaseThresh = (IncreaseThresh > 0) ? IncreaseThresh - 1 : 0;

    if (BusyPercentage >= IncreaseThresh)
    {
        /* CPU is highly loaded: snap to maximum performance */
        Target = MaxThrottle;
    }
    else if (BusyPercentage <= DecreaseThresh)
    {
        /* CPU is lightly loaded: snap to minimum performance */
        Target = MinThrottle;
    }
    else
    {
        /*
         * CPU is in the neutral band: interpolate linearly between
         * MinThrottle and MaxThrottle based on position within the band.
         *
         *   target = Min + (busy - DecrThresh) / (IncrThresh - DecrThresh)
         *                × (Max - Min)
         */
        BandRange = IncreaseThresh - DecreaseThresh;
        Target = (UCHAR)(MinThrottle +
                         ((ULONG)(BusyPercentage - DecreaseThresh) *
                          (MaxThrottle - MinThrottle) + BandRange / 2) /
                         BandRange);
    }

    /* Final clamp: always respect the per-processor limits */
    if (Target < MinThrottle)
        Target = MinThrottle;
    if (Target > MaxThrottle)
        Target = MaxThrottle;

    PPMTRACE(PPM_PERF_DEBUG,
             "PpmSelectPerfState: CPU %u busy=%u%% → throttle=%u%%"
             " (min=%u, max=%u, inc_thresh=%u, dec_thresh=%u)\n",
             Prcb->Number, BusyPercentage, Target,
             MinThrottle, MaxThrottle, IncreaseThresh, DecreaseThresh);

    return Target;
}

/**
 * @brief
 * Applies the requested throttle percentage @p ThrottlePercent to the
 * current processor.
 *
 * @param[in,out] PowerState
 * Per-processor power state block.
 *
 * @param[in] ThrottlePercent
 * Target throttle level in [0, 100].  100 = full speed, 0 = minimum.
 *
 * @remarks
 * Two paths are attempted in order (aligned with Windows behaviour):
 *
 *  1. If the processor driver registered a PerfSetThrottle callback via the
 *     legacy PROCESSOR_STATE_HANDLER2 path (PowerState->PerfSetThrottle ≠ NULL)
 *     that callback is invoked.
 *
 *  2. If RegisterPerfStates supplied PerfSelectionHandler and
 *     PerfControlHandler (Windows PROCESSOR_PERF_STATES layout), run
 *     selection then control on the current processor.
 *
 *  3. Otherwise HalSetSystemInformation(HalProcessorSpeedInformation, …).
 *
 * If no path applies, only CurrentThrottle is updated.
 */
VOID
NTAPI
PpmApplyThrottle(
    _In_ PPROCESSOR_POWER_STATE PowerState,
    _In_ UCHAR                  ThrottlePercent)
{
    typedef NTSTATUS (NTAPI *PSET_THROTTLE_FN)(UCHAR Throttle);
    PSET_THROTTLE_FN SetThrottle;
    PPROCESSOR_PERF_STATES PerfStates;

    /* Short-circuit if the throttle level hasn't changed */
    if (PowerState->CurrentThrottle == ThrottlePercent)
        return;

    SetThrottle = (PSET_THROTTLE_FN)PowerState->PerfSetThrottle;

    if (SetThrottle != NULL)
    {
        /*
         * Use the driver-registered legacy throttle callback.  This is the
         * path used by ACPI-based intelppm / amdppm legacy registrations.
         */
        SetThrottle(ThrottlePercent);
    }
    else
    {
        PerfStates = (PPROCESSOR_PERF_STATES)PowerState->IdleHandlers;
        if (PerfStates != NULL &&
            PerfStates->Version == PROCESSOR_PERF_STATES_VERSION &&
            PerfStates->PerfControlHandler != NULL &&
            PerfStates->PerfSelectionHandler != NULL)
        {
            ULONG Frequency = 0;
            ULONGLONG Selection = 0;

            PerfStates->PerfSelectionHandler(
                PerfStates->GlobalContext,
                ThrottlePercent,
                PowerState->ProcessorMinThrottle,
                PowerState->ProcessorMaxThrottle,
                0,
                &Frequency,
                &Selection);

            PerfStates->PerfControlHandler(
                PerfStates->GlobalContext,
                Selection,
                PowerState->ProcessorMinThrottle,
                PowerState->ProcessorMaxThrottle,
                2, /* tolerance % — matches typical Windows default order-of-magnitude */
                0, /* autonomous */
                1, /* initiate */
                0); /* force */

            PPMTRACE(PPM_PERF_DEBUG,
                     "PpmApplyThrottle: CPU %u PerfStates control sel=%lu freq=%lu\n",
                     KeGetCurrentProcessorNumber(),
                     (ULONG)Selection,
                     Frequency);
        }
        else
        {
            HAL_PROCESSOR_SPEED_INFORMATION SpeedInfo;

            SpeedInfo.ProcessorSpeed = (ULONG)ThrottlePercent;
            HalSetSystemInformation(HalProcessorSpeedInformation,
                                    sizeof(SpeedInfo),
                                    &SpeedInfo);
            PPMTRACE(PPM_PERF_DEBUG,
                     "PpmApplyThrottle: CPU %u HAL throttle %u%%\n",
                     KeGetCurrentProcessorNumber(), ThrottlePercent);
        }
    }

    PowerState->CurrentThrottle = ThrottlePercent;

    PPMTRACE(PPM_PERF_DEBUG,
             "PpmApplyThrottle: CPU %u → %u%%\n",
             KeGetCurrentProcessorNumber(), ThrottlePercent);
}

/* EOF */

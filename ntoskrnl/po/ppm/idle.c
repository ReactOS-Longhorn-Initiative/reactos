/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Processor Power Management – idle processor handling
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 *              Copyright 2025 ReactOS Contributors
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * _PpmEstimateIdleDuration
 *
 * Returns a rough estimate of how long the processor will remain idle, in
 * 100-ns interrupt-time units.
 *
 * The estimate is derived from the average time between the last two idle
 * periods (stored in PowerState->LastCheck vs the current interrupt time).
 * A value of 0 means "unknown / too short to measure" and causes
 * PpmSelectIdleState to ask the driver's IdlePreselect callback.
 */
static FORCEINLINE
ULONGLONG
_PpmEstimateIdleDuration(
    _In_ PPROCESSOR_POWER_STATE PowerState)
{
    ULONGLONG Now = KeQueryInterruptTime();
    ULONGLONG Delta;

    if (PowerState->LastCheck == 0)
    {
        PowerState->LastCheck = Now;
        return 0;
    }

    Delta = Now - PowerState->LastCheck;
    PowerState->LastCheck = Now;

    /* Ignore unreasonably large deltas (processor just resumed from sleep) */
    if (Delta > 10000000ULL) /* > 1 second */
        Delta = 0;

    return Delta;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Kernel idle function for PPM-managed processors.
 *
 * @param[in] PowerState
 * Per-processor power state block (Prcb->PowerState).
 *
 * @remarks
 * This function is installed into each KPRCB::PowerState.IdleFunction by
 * PpmInitialize() during the early phase and is called by the kernel idle
 * loop (KiIdleLoop) whenever a processor has no runnable threads.
 *
 * Execution flow
 * ──────────────
 *  1. Estimate the upcoming idle duration from the inter-idle interval
 *     (eng.c → _PpmEstimateIdleDuration).
 *
 *  2. Ask the core engine which C-state is appropriate
 *     (eng.c → PpmSelectIdleState).
 *
 *  3a. If a driver-provided C-state table is available:
 *       – Call the driver's IdlePreExecute callback to give it a chance to
 *         set up hardware.
 *       – Call IdleExecute to actually transition the processor.
 *       – On return (wake event), call IdleComplete.
 *       – Record the idle duration in cpustat.c → PpmUpdateIdleAccounting.
 *
 *  3b. If no driver table is registered: fall back to HalProcessorIdle()
 *      which executes a single HLT instruction.
 *
 * Thread safety
 * ─────────────
 * Each processor runs this function only on itself, so all fields accessed
 * (PowerState, Prcb counters) are processor-local and require no locking.
 */
VOID
FASTCALL
PpmIdle(
    _In_ PPROCESSOR_POWER_STATE PowerState)
{
    PPPM_IDLE_STATES_EX IdleStates;
    ULONGLONG           IdleDuration;
    ULONGLONG           EntryTime;
    ULONGLONG           ExitTime;
    ULONG               StateIndex;
    ULONG               Hint;
    NTSTATUS            Status;

    /* --- Step 1: Estimate idle duration --------------------------------- */
    IdleDuration = _PpmEstimateIdleDuration(PowerState);

    /* --- Step 2: C-state selection -------------------------------------- */
    StateIndex = PpmSelectIdleState(PowerState, IdleDuration);

    /* MAXULONG means no driver table is registered */
    if (StateIndex == MAXULONG)
    {
        /* Legacy path: single-instruction halt via HAL */
        HalProcessorIdle();
        return;
    }

    IdleStates = (PPPM_IDLE_STATES_EX)PowerState->IdleState;

    /* Clamp to valid range */
    if (StateIndex >= IdleStates->ProcessorIdleCount)
        StateIndex = IdleStates->ProcessorIdleCount - 1;

    /* --- Step 3a: Enter driver-managed C-state -------------------------- */

    EntryTime = KeQueryInterruptTime();

    PowerState->IdleTimes.StartTime = EntryTime;

    /*
     * IdlePreExecute gives the driver a chance to flush write buffers,
     * set MWAIT address hints, or record timestamps.
     */
    Hint = 0;
    if (IdleStates->IdlePreExecute != NULL)
    {
        Status = IdleStates->IdlePreExecute(
                     IdleStates->Context,
                     StateIndex,
                     KeGetCurrentProcessorNumber(),
                     0,
                     &Hint);

        if (!NT_SUCCESS(Status))
        {
            /*
             * Pre-execute rejected – fall back to HalProcessorIdle rather
             * than entering an unknown hardware state.
             */
            PPMTRACE(PPM_PERF_DEBUG,
                     "PpmIdle: CPU %u IdlePreExecute(state=%lu) failed 0x%08lx;"
                     " falling back to HAL\n",
                     KeGetCurrentProcessorNumber(), StateIndex, Status);

            HalProcessorIdle();
            return;
        }
    }

    /*
     * IdleExecute performs the actual low-power transition (HLT, MWAIT, or
     * a platform-specific firmware call).  Control returns here on any wake
     * event (interrupt, NMI, SMI).
     */
    if (IdleStates->IdleExecute != NULL)
    {
        Status = IdleStates->IdleExecute(
                     IdleStates->Context,
                     StateIndex,
                     KeGetCurrentProcessorNumber(),
                     0,
                     &Hint);

        if (!NT_SUCCESS(Status))
        {
            PPMTRACE(PPM_PERF_DEBUG,
                     "PpmIdle: CPU %u IdleExecute(state=%lu) failed 0x%08lx\n",
                     KeGetCurrentProcessorNumber(), StateIndex, Status);
        }
    }
    else
    {
        /* No execute callback; use HAL as a safe fallback */
        HalProcessorIdle();
    }

    /* --- Step 3a continued: Record wake time and update accounting ------- */

    ExitTime = KeQueryInterruptTime();
    PowerState->IdleTimes.EndTime = ExitTime;

    /* Notify the driver that the idle period has ended */
    if (IdleStates->IdleComplete != NULL)
    {
        IdleStates->IdleComplete(
            IdleStates->Context,
            StateIndex,
            KeGetCurrentProcessorNumber(),
            0,
            &Hint);
    }

    /* Update per-state residency counters */
    if (ExitTime > EntryTime)
    {
        PpmUpdateIdleAccounting(PowerState,
                                StateIndex,
                                ExitTime - EntryTime);
    }

    /*
     * Track C3 percentage for thermal policy.
     * State index 2 corresponds to C3 in the standard 3-state table.
     */
    if (StateIndex >= 2 && (ExitTime > EntryTime))
    {
        PowerState->PreviousC3StateTime += (ExitTime - EntryTime);
    }
}

/* EOF */

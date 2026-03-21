/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Processor Power Management Initialization Code
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
 * Initializes the Processor Power Management (PPM) subsystem.
 *
 * @param[in] EarlyPhase
 * If TRUE, this is the early initialization pass (Phase 1, called before
 * drivers are loaded). If FALSE, this is the late initialization pass
 * (Phase 2), called after the I/O subsystem is fully initialized.
 *
 * @return
 * Returns STATUS_SUCCESS if initialization was successful.
 *
 * @remarks
 * Early phase:
 *   - Hooks up the PpmIdle function to each processor's KPRCB.PowerState so
 *     that the kernel idle loop calls it instead of the generic HalProcessorIdle.
 *   - Initializes per-processor power state fields (throttle limits, etc.).
 *
 * Late phase:
 *   - Starts the periodic performance-monitoring DPC timer on each processor.
 *   - Samples the high-resolution performance counter frequency that the DPC
 *     routine uses to compute processor utilization.
 */
CODE_SEG("INIT")
NTSTATUS
NTAPI
PpmInitialize(
    _In_ BOOLEAN EarlyPhase)
{
    ULONG ProcessorIndex;
    ULONG ProcessorCount;
    PKPRCB Prcb;
    PPROCESSOR_POWER_STATE PowerState;
    LARGE_INTEGER PerfFrequency;
    LARGE_INTEGER DpcPeriod;

    PAGED_CODE();

    ProcessorCount = (ULONG)KeNumberProcessors;

    if (EarlyPhase)
    {
        /*
         * Populate the global PPM kernel dispatch table that processor drivers
         * (intelppm, amdppm) obtain via ZwPowerInformation(ProcessorStateHandler).
         * This must happen before any processor driver is loaded.
         */
        PpmInitDispatchTable();

        /*
         * Early phase: configure each processor's power state before any
         * driver or user-mode thread can execute on them.
         */
        for (ProcessorIndex = 0; ProcessorIndex < ProcessorCount; ProcessorIndex++)
        {
            Prcb = KiProcessorBlock[ProcessorIndex];
            if (Prcb == NULL)
            {
                continue;
            }

            PowerState = &Prcb->PowerState;

            /*
             * Install PpmIdle as the processor's idle function. The kernel
             * idle loop (KiIdleLoop) calls PowerState->IdleFunction once it
             * determines the processor has nothing to execute. PpmIdle is
             * responsible for transitioning the processor into the deepest
             * C state that policy allows and for collecting idle-time metrics.
             */
            PowerState->IdleFunction = PpmIdle;

            /*
             * Throttle limits: start at 100 % (no throttling). When PPM
             * policy evaluation determines throttling is necessary it will
             * lower CurrentThrottle via the HAL throttle interface.
             */
            PowerState->CurrentThrottle = 100;
            PowerState->ThermalThrottleLimit = 100;
            PowerState->ProcessorMinThrottle = 0;
            PowerState->ProcessorMaxThrottle = 100;

            PPMTRACE(PPM_INIT_SUBSYSTEM_DEBUG,
                     "PpmInitialize (early): processor %lu IdleFunction set to PpmIdle\n",
                     ProcessorIndex);
        }
    }
    else
    {
        /*
         * Late phase: the HAL and all boot drivers are loaded. We can now
         * query the performance counter frequency and arm the per-processor
         * performance-monitoring DPC timer.
         */
        KeQueryPerformanceCounter(&PerfFrequency);

        /*
         * Negative value → relative timer (fire PPM_PERF_DPC_PERIOD 100-ns
         * units from now).  KeSetTimerEx accepts the period in milliseconds
         * as its last argument for the recurring interval.
         */
        DpcPeriod.QuadPart = -(LONGLONG)PPM_PERF_DPC_PERIOD;

        for (ProcessorIndex = 0; ProcessorIndex < ProcessorCount; ProcessorIndex++)
        {
            Prcb = KiProcessorBlock[ProcessorIndex];
            if (Prcb == NULL)
            {
                continue;
            }

            PowerState = &Prcb->PowerState;

            /* Store the performance counter frequency for idle ratio computation */
            PowerState->PerfCounterFrequency = PerfFrequency;

            /*
             * Initialize and arm the per-processor performance-monitoring DPC
             * timer. PpmPerfIdleDpcRoutine fires periodically to sample
             * processor utilization and select the appropriate P-state.
             *
             * The DPC and timer are already embedded in PROCESSOR_POWER_STATE
             * (PerfDpc and PerfTimer), so we only need to initialise and set them.
             */
            KeInitializeDpc(&PowerState->PerfDpc,
                            PpmPerfIdleDpcRoutine,
                            Prcb);
            KeSetTargetProcessorDpc(&PowerState->PerfDpc, (CCHAR)ProcessorIndex);
            KeSetImportanceDpc(&PowerState->PerfDpc, MediumImportance);

            KeInitializeTimerEx(&PowerState->PerfTimer, SynchronizationTimer);

            /*
             * Arm the recurring DPC timer.  The timer fires once per
             * PPM_PERF_DPC_PERIOD_MS (20 ms), which is the standard Windows
             * performance-sampling interval for the PPM policy engine.
             *
             * KeSetTimerEx period parameter is in milliseconds.
             */
            KeSetTimerEx(&PowerState->PerfTimer,
                         DpcPeriod,
                         PPM_PERF_DPC_PERIOD_MS,
                         &PowerState->PerfDpc);

            PPMTRACE(PPM_INIT_SUBSYSTEM_DEBUG,
                     "PpmInitialize (late): processor %lu perf DPC/timer armed"
                     " (period=%d ms)\n",
                     ProcessorIndex, PPM_PERF_DPC_PERIOD_MS);
        }
    }

    return STATUS_SUCCESS;
}

/* EOF */

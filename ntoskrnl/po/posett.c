/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager power settings callback mechanism
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <internal/ppm.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY PopPowerSettingCallbacksList;
ULONG PopPowerSettingCallbacksCount;
PKTHREAD PopPowerSettingOwnerLockThread;
FAST_MUTEX PopPowerSettingLock;

/*
 * Note: The system-wide thermal cooling mode global is PopCoolingSystemMode,
 * which is defined in thermzn.c and declared via po.h.
 * 0 = passive cooling (throttle processor first before spinning fans).
 * 1 = active cooling  (spin fans aggressively; used on desktops / AC-power).
 *
 * PopSystemCoolingPolicySettingWorker (below) updates PopCoolingSystemMode
 * when GUID_SYSTEM_COOLING_POLICY changes.
 */

/* FORWARD DECLARATIONS FOR PROCESSOR POWER SETTING WORKERS ******************/

static VOID NTAPI PopProcessorThrottleMaximumSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopProcessorThrottleMinimumSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopProcessorPerfIncreaseThresholdSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopProcessorPerfDecreaseThresholdSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopProcessorPerfIncreaseTimeSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopProcessorPerfDecreaseTimeSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopProcessorIdleDemoteThresholdSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopProcessorIdlePromoteThresholdSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopProcessorCoreParkingSettingWorker(_In_ PVOID StartContext);
static VOID NTAPI PopSystemCoolingPolicySettingWorker(_In_ PVOID StartContext);

/* POWER SETTINGS DATABASE ****************************************************/

POP_POWER_SETTING_DATABASE PopPowerSettingsDatabase[POP_MAX_POWER_SETTINGS] =
{
    {&GUID_MAX_POWER_SAVINGS, NULL},
    {&GUID_MIN_POWER_SAVINGS, NULL},
    {&GUID_TYPICAL_POWER_SAVINGS, NULL},
    {&NO_SUBGROUP_GUID, NULL},
    {&ALL_POWERSCHEMES_GUID, NULL},
    {&GUID_POWERSCHEME_PERSONALITY, NULL},
    {&GUID_ACTIVE_POWERSCHEME, NULL},
    {&GUID_VIDEO_POWERDOWN_TIMEOUT, NULL},
    {&GUID_VIDEO_ANNOYANCE_TIMEOUT, NULL},
    {&GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE, NULL},
    {&GUID_VIDEO_DIM_TIMEOUT, NULL},
    {&GUID_VIDEO_ADAPTIVE_POWERDOWN, NULL},
    {&GUID_MONITOR_POWER_ON, NULL},
    {&GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, NULL},
    {&GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS, NULL},
    {&GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS, NULL},
    {&GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS, NULL},
    {&GUID_SESSION_DISPLAY_STATE, NULL},
    {&GUID_CONSOLE_DISPLAY_STATE, NULL},
    {&GUID_ALLOW_DISPLAY_REQUIRED, NULL},
    {&GUID_DISK_SUBGROUP, NULL},
    {&GUID_DISK_POWERDOWN_TIMEOUT, NULL},
    {&GUID_DISK_IDLE_TIMEOUT, NULL},
    {&GUID_DISK_BURST_IGNORE_THRESHOLD, NULL},
    {&GUID_DISK_ADAPTIVE_POWERDOWN, NULL},
    {&GUID_SLEEP_SUBGROUP, NULL},
    {&GUID_SLEEP_IDLE_THRESHOLD, NULL},
    {&GUID_STANDBY_TIMEOUT, NULL},
    {&GUID_UNATTEND_SLEEP_TIMEOUT, NULL},
    {&GUID_HIBERNATE_TIMEOUT, NULL},
    {&GUID_HIBERNATE_FASTS4_POLICY, NULL},
    {&GUID_CRITICAL_POWER_TRANSITION, NULL},
    {&GUID_SYSTEM_AWAYMODE, NULL},
    {&GUID_ALLOW_AWAYMODE, NULL},
    {&GUID_ALLOW_STANDBY_STATES, NULL},
    {&GUID_ALLOW_RTC_WAKE, NULL},
    {&GUID_ALLOW_SYSTEM_REQUIRED, NULL},
    {&GUID_SYSTEM_BUTTON_SUBGROUP, NULL},
    {&GUID_POWERBUTTON_ACTION, NULL},
    {&GUID_POWERBUTTON_ACTION_FLAGS, NULL},
    {&GUID_SLEEPBUTTON_ACTION, NULL},
    {&GUID_SLEEPBUTTON_ACTION_FLAGS, NULL},
    {&GUID_USERINTERFACEBUTTON_ACTION, NULL},
    {&GUID_LIDCLOSE_ACTION, NULL},
    {&GUID_LIDCLOSE_ACTION_FLAGS, NULL},
    {&GUID_LIDOPEN_POWERSTATE, NULL},
    {&GUID_BATTERY_SUBGROUP, NULL},
    {&GUID_BATTERY_DISCHARGE_ACTION_0, NULL},
    {&GUID_BATTERY_DISCHARGE_LEVEL_0, NULL},
    {&GUID_BATTERY_DISCHARGE_FLAGS_0, NULL},
    {&GUID_BATTERY_DISCHARGE_ACTION_1, NULL},
    {&GUID_BATTERY_DISCHARGE_LEVEL_1, NULL},
    {&GUID_BATTERY_DISCHARGE_FLAGS_1, NULL},
    {&GUID_BATTERY_DISCHARGE_ACTION_2, NULL},
    {&GUID_BATTERY_DISCHARGE_LEVEL_2, NULL},
    {&GUID_BATTERY_DISCHARGE_FLAGS_2, NULL},
    {&GUID_BATTERY_DISCHARGE_ACTION_3, NULL},
    {&GUID_BATTERY_DISCHARGE_LEVEL_3, NULL},
    {&GUID_BATTERY_DISCHARGE_FLAGS_3, NULL},
    {&GUID_PROCESSOR_SETTINGS_SUBGROUP, NULL},
    {&GUID_PROCESSOR_THROTTLE_POLICY, NULL},
    {&GUID_PROCESSOR_THROTTLE_MAXIMUM, PopProcessorThrottleMaximumSettingWorker},
    {&GUID_PROCESSOR_THROTTLE_MINIMUM, PopProcessorThrottleMinimumSettingWorker},
    {&GUID_PROCESSOR_ALLOW_THROTTLING, NULL},
    {&GUID_PROCESSOR_IDLESTATE_POLICY, NULL},
    {&GUID_PROCESSOR_PERFSTATE_POLICY, NULL},
    {&GUID_PROCESSOR_PERF_INCREASE_THRESHOLD, PopProcessorPerfIncreaseThresholdSettingWorker},
    {&GUID_PROCESSOR_PERF_DECREASE_THRESHOLD, PopProcessorPerfDecreaseThresholdSettingWorker},
    {&GUID_PROCESSOR_PERF_INCREASE_POLICY, NULL},
    {&GUID_PROCESSOR_PERF_DECREASE_POLICY, NULL},
    {&GUID_PROCESSOR_PERF_INCREASE_TIME, PopProcessorPerfIncreaseTimeSettingWorker},
    {&GUID_PROCESSOR_PERF_DECREASE_TIME, PopProcessorPerfDecreaseTimeSettingWorker},
    {&GUID_PROCESSOR_PERF_TIME_CHECK, NULL},
    {&GUID_PROCESSOR_PERF_BOOST_POLICY, NULL},
    {&GUID_PROCESSOR_IDLE_ALLOW_SCALING, NULL},
    {&GUID_PROCESSOR_IDLE_DISABLE, NULL},
    {&GUID_PROCESSOR_IDLE_TIME_CHECK, NULL},
    {&GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD, PopProcessorIdleDemoteThresholdSettingWorker},
    {&GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD, PopProcessorIdlePromoteThresholdSettingWorker},
    {&GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD, PopProcessorCoreParkingSettingWorker},
    {&GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD, PopProcessorCoreParkingSettingWorker},
    {&GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_MAX_CORES, PopProcessorCoreParkingSettingWorker},
    {&GUID_PROCESSOR_CORE_PARKING_MIN_CORES, PopProcessorCoreParkingSettingWorker},
    {&GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME, PopProcessorCoreParkingSettingWorker},
    {&GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME, PopProcessorCoreParkingSettingWorker},
    {&GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD, NULL},
    {&GUID_PROCESSOR_PARKING_CORE_OVERRIDE, NULL},
    {&GUID_PROCESSOR_PARKING_PERF_STATE, NULL},
    {&GUID_PROCESSOR_PERF_HISTORY, NULL},
    {&GUID_SYSTEM_COOLING_POLICY, PopSystemCoolingPolicySettingWorker},
    {&GUID_LOCK_CONSOLE_ON_WAKE, NULL},
    {&GUID_DEVICE_IDLE_POLICY, NULL},
    {&GUID_ACDC_POWER_SOURCE, PopPowerSourceSettingWorker},
    {&GUID_LIDSWITCH_STATE_CHANGE, PopLidStateChangeSettingWorker},
    {&GUID_BATTERY_PERCENTAGE_REMAINING, PopBatteryRemainingSettingWorker},
    {&GUID_IDLE_BACKGROUND_TASK, NULL},
    {&GUID_BACKGROUND_TASK_NOTIFICATION, NULL},
    {&GUID_APPLAUNCH_BUTTON, NULL},
    {&GUID_PCIEXPRESS_SETTINGS_SUBGROUP, NULL},
    {&GUID_PCIEXPRESS_ASPM_POLICY, NULL},
    {&GUID_ENABLE_SWITCH_FORCED_SHUTDOWN, NULL},
};

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Main worker thread for the GUID_ACDC_POWER_SOURCE power setting.
 * It determines the currently enforced power policy and sets the
 * new power condiition setting value to the registered driver.
 *
 * @param[in] StartContext
 * A pointer to a start argument context. This points to the setting
 * callback that has registered for this power setting.
 */
_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopPowerSourceSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    SYSTEM_POWER_CONDITION PowerCondition;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);

    /* Sanity check: the following worker can only work with the right GUID callbacks */
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_ACDC_POWER_SOURCE) == TRUE);

    /*
     * No callback must be executing at the time of calling this worker
     * to handle this power setting.
     */
    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    /*
     * Determine how is this system actually powered up. It can either be
     * powered up by short-term batteries such as a UPS, in this case we have
     * to explicitly check for that against the reported system capabilities.
     * Otherwise just check the default system power policy the Power Manager
     * has currently enforced.
     */
    if (PopCapabilities.BatteriesAreShortTerm)
    {
        PowerCondition = PoHot;
    }
    else if (PopDefaultPowerPolicy == &PopAcPowerPolicy)
    {
        PowerCondition = PoAc;
    }
    else // PopDefaultPowerPolicy == &PopDcPowerPolicy
    {
        PowerCondition = PoDc;
    }

    /*
     * This power setting is notified, acknowledge the Power Manager
     * that this power setting is entering into the driver's callback
     * to deliver the state change of the power setting.
     */
    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    /* Execute the driver's supplied callback */
    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &PowerCondition,
                                       sizeof(SYSTEM_POWER_CONDITION),
                                       SettingCallback->Context);

    /* Log to the debugger what happened with the callback */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n", SettingCallback, Status);
    }

    /* Operation done, alert the Power Manager this power setting has finished its job */
    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_LIDSWITCH_STATE_CHANGE power setting.
 * It determines whether the lid is open or closed of a portable machine,
 * such as a laptop.
 *
 * @param[in] StartContext
 * A pointer to a start argument context. This points to the setting
 * callback that has registered for this power setting.
 */
_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopLidStateChangeSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG LidIsOpen;
    PLIST_ENTRY Entry;
    PPOP_CONTROL_SWITCH ControlSwitch;
    BOOLEAN LidFound = FALSE;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);

    /* Sanity check: the following worker can only work with the right GUID callbacks */
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_LIDSWITCH_STATE_CHANGE) == TRUE);

    /*
     * No callback must be executing at the time of calling this worker
     * to handle this power setting.
     */
    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    /*
     * Acquire the policy lock here as we are going to loop inside the
     * list of control switches and look for the exact switch that is
     * a lid.
     */
    PopAcquirePowerPolicyLock();
    for (Entry = PopControlSwitches.Flink;
         Entry != &PopControlSwitches;
         Entry = Entry->Flink)
    {
        ControlSwitch = CONTAINING_RECORD(Entry, POP_CONTROL_SWITCH, Link);
        if (ControlSwitch->SwitchType == SwitchLid)
        {
            LidFound = TRUE;
            break;
        }
    }

    /* Do not execute the driver's callback if no lid was found on this system */
    PopReleasePowerPolicyLock();
    if (!LidFound)
    {
        DPRINT("No lid found, the driver's callback won't be called\n");
        PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
        PsTerminateSystemThread(STATUS_SUCCESS);
        return;
    }

    /* Cache the lid's status and prepare to give it to the driver's callback */
    LidIsOpen = ControlSwitch->Switch.Lid.Opened;

    /*
     * This power setting is notified, acknowledge the Power Manager
     * that this power setting is entering into the driver's callback
     * to deliver the state change of the power setting.
     */
    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    /* Execute the driver's supplied callback */
    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &LidIsOpen,
                                       sizeof(ULONG),
                                       SettingCallback->Context);

    /* Log to the debugger what happened with the callback */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n", SettingCallback, Status);
    }

    /* Operation done, alert the Power Manager this power setting has finished its job */
    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_BATTERY_PERCENTAGE_REMAINING power setting.
 * It determines the current percentage remaining of the battery capacity.
 *
 * @param[in] StartContext
 * A pointer to a start argument context. This points to the setting
 * callback that has registered for this power setting.
 */
_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopBatteryRemainingSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG CapacityRemaining, CurrentCapacity, MaxCapacity;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);

    /* Sanity check: the following worker can only work with the right GUID callbacks */
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_BATTERY_PERCENTAGE_REMAINING) == TRUE);

    /*
     * No callback must be executing at the time of calling this worker
     * to handle this power setting.
     */
    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    /*
     * The system is powered by an external AC adapter with battery
     * fully charged or AC source (for example from the PSU).
     * In such case assume the capacity remaining is 100%.
     */
    if (PopDefaultPowerPolicy == &PopAcPowerPolicy)
    {
        CapacityRemaining = 100;
    }
    else
    {
        /* The system is powered by batteries (aka DC source), figure out the remaining capacity */
        CurrentCapacity = PopBattery->Status.Capacity;
        MaxCapacity = PopBattery->BattInfo.FullChargedCapacity;

        /*
         * Compute the exact percentage of remaining battery capacity only if the
         * current capacity at the time of reading is below the reported maximum
         * capacity of the battery. Otherwise return 100%.
         */
        if (CurrentCapacity <= MaxCapacity)
        {
            CapacityRemaining = (ULONG)((100 * CurrentCapacity + MaxCapacity / 2) / MaxCapacity);
        }
        else
        {
            CapacityRemaining = 100;
        }
    }

    /*
     * This power setting is notified, acknowledge the Power Manager
     * that this power setting is entering into the driver's callback
     * to deliver the state change of the power setting.
     */
    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    /* Execute the driver's supplied callback */
    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &CapacityRemaining,
                                       sizeof(ULONG),
                                       SettingCallback->Context);

    /* Log to the debugger what happened with the callback */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n", SettingCallback, Status);
    }

    /* Operation done, alert the Power Manager this power setting has finished its job */
    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/* PROCESSOR POWER SETTING WORKERS *******************************************/

/*
 * Helper: iterate over every active logical processor and propagate a
 * throttle limit change to its per-PRCB power-state block.
 *
 * Must be called at PASSIVE_LEVEL.
 */
static
VOID
PpmApplyThrottleLimitsToAllProcessors(VOID)
{
    ULONG ProcessorCount;
    ULONG ProcessorIndex;
    PKPRCB Prcb;
    PPROCESSOR_POWER_STATE PowerState;
    UCHAR NewMax;
    UCHAR NewMin;

    ProcessorCount = (ULONG)KeNumberProcessors;
    NewMax = PpmPolicyMaxThrottle;
    NewMin = PpmPolicyMinThrottle;

    /* Clamp to avoid a floor above the ceiling */
    if (NewMin > NewMax)
        NewMin = NewMax;

    for (ProcessorIndex = 0; ProcessorIndex < ProcessorCount; ProcessorIndex++)
    {
        Prcb = KiProcessorBlock[ProcessorIndex];
        if (Prcb == NULL)
            continue;

        PowerState = &Prcb->PowerState;

        PowerState->ProcessorMaxThrottle = NewMax;
        PowerState->ProcessorMinThrottle = NewMin;

        /* If the processor is currently running above the new ceiling,
         * bring it down immediately so the new limit is effective at once. */
        if (PowerState->CurrentThrottle > NewMax)
            PpmApplyThrottle(PowerState, NewMax);
        else if (PowerState->CurrentThrottle < NewMin)
            PpmApplyThrottle(PowerState, NewMin);
    }
}

/**
 * @brief
 * Main worker thread for the GUID_PROCESSOR_THROTTLE_MAXIMUM power setting.
 * Updates the maximum processor throttle (performance ceiling) that the PPM
 * engine may select when the system is under load.
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorThrottleMaximumSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG ThrottleMax;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_PROCESSOR_THROTTLE_MAXIMUM) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    /*
     * Read the maximum throttle from the PPM policy global.
     * PpmPolicyMaxThrottle is the authoritative ceiling for P-state selection.
     * It defaults to 100 % and may be capped by RegisterPerfCap callbacks
     * when a processor driver reports a BIOS or thermal performance limit.
     *
     * Note: SYSTEM_POWER_POLICY does not carry a per-GUID throttle-maximum
     * field; the ForcedThrottle field describes the emergency overclock cap
     * applied when the system is overheating, not the steady-state ceiling.
     */
    ThrottleMax = (ULONG)PpmPolicyMaxThrottle;

    /*
     * Push the current ceiling to every active processor's PRCB so that
     * the PPM policy engine picks up the change on the next DPC period.
     */
    PpmApplyThrottleLimitsToAllProcessors();

    DPRINT("PopProcessorThrottleMaximumSettingWorker: max=%lu%%\n", ThrottleMax);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &ThrottleMax,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_PROCESSOR_THROTTLE_MINIMUM power setting.
 * Updates the minimum processor throttle (performance floor) to prevent the
 * PPM engine from lowering the frequency below a policy-defined baseline.
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorThrottleMinimumSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG ThrottleMin;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_PROCESSOR_THROTTLE_MINIMUM) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    /*
     * SYSTEM_POWER_POLICY.MinThrottle is the minimum throttle percentage
     * stored in the active policy structure (loaded from registry by
     * PopDefaultPolicies).  This field maps directly to
     * GUID_PROCESSOR_THROTTLE_MINIMUM: it is the floor below which the
     * P-state engine should not lower the processor clock.
     *
     * If no policy is active yet use the current PPM global default (0 %).
     */
    if (PopDefaultPowerPolicy != NULL)
        ThrottleMin = (ULONG)PopDefaultPowerPolicy->MinThrottle;
    else
        ThrottleMin = (ULONG)PpmPolicyMinThrottle;

    if (ThrottleMin > 100)
        ThrottleMin = 100;

    /* Update the PPM engine's policy knob */
    PpmPolicyMinThrottle = (UCHAR)ThrottleMin;
    PpmApplyThrottleLimitsToAllProcessors();

    DPRINT("PopProcessorThrottleMinimumSettingWorker: min=%lu%%\n", ThrottleMin);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &ThrottleMin,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_PROCESSOR_PERF_INCREASE_THRESHOLD power
 * setting.  Updates the busy-percentage level above which the PPM engine
 * considers promoting to a higher performance state.
 *
 * The value is a DWORD in the range [0, 100] representing a percentage.
 * Default: 60 % (set during driver initialisation in dispatch.c).
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorPerfIncreaseThresholdSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG Threshold;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid,
                          &GUID_PROCESSOR_PERF_INCREASE_THRESHOLD) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    /* Use the current global value; external code may already have updated it */
    Threshold = PpmPolicyPerfIncreaseThreshold;

    DPRINT("PopProcessorPerfIncreaseThresholdSettingWorker: threshold=%lu%%\n",
           Threshold);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &Threshold,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_PROCESSOR_PERF_DECREASE_THRESHOLD power
 * setting.  Updates the busy-percentage level below which the PPM engine
 * considers demoting to a lower performance state.
 *
 * Default: 40 % (set during driver initialisation in dispatch.c).
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorPerfDecreaseThresholdSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG Threshold;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid,
                          &GUID_PROCESSOR_PERF_DECREASE_THRESHOLD) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    Threshold = PpmPolicyPerfDecreaseThreshold;

    DPRINT("PopProcessorPerfDecreaseThresholdSettingWorker: threshold=%lu%%\n",
           Threshold);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &Threshold,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_PROCESSOR_PERF_INCREASE_TIME power setting.
 * Updates the number of consecutive PPM DPC periods the processor must remain
 * above the increase threshold before the P-state is promoted.
 *
 * The value is a DWORD (number of DPC periods, each 20 ms).
 * Default: 1 period (fast ramp-up).
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorPerfIncreaseTimeSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG IncreaseTime;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid,
                          &GUID_PROCESSOR_PERF_INCREASE_TIME) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    IncreaseTime = PpmPolicyPerfIncreaseTime;

    DPRINT("PopProcessorPerfIncreaseTimeSettingWorker: periods=%lu\n",
           IncreaseTime);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &IncreaseTime,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_PROCESSOR_PERF_DECREASE_TIME power setting.
 * Updates the number of consecutive PPM DPC periods the processor must remain
 * below the decrease threshold before the P-state is demoted.
 *
 * The value is a DWORD (number of DPC periods, each 20 ms).
 * Default: 5 periods (slow ramp-down, avoids oscillation on bursty loads).
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorPerfDecreaseTimeSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG DecreaseTime;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid,
                          &GUID_PROCESSOR_PERF_DECREASE_TIME) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    DecreaseTime = PpmPolicyPerfDecreaseTime;

    DPRINT("PopProcessorPerfDecreaseTimeSettingWorker: periods=%lu\n",
           DecreaseTime);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &DecreaseTime,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD power
 * setting.  Updates the idle-time percentage below which the PPM engine
 * considers moving the processor to a shallower C-state.
 *
 * Default: 10 % idle time.
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorIdleDemoteThresholdSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG Threshold;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid,
                          &GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    Threshold = PpmPolicyIdleDemoteThreshold;

    DPRINT("PopProcessorIdleDemoteThresholdSettingWorker: threshold=%lu%%\n",
           Threshold);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &Threshold,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD power
 * setting.  Updates the idle-time percentage above which the PPM engine
 * considers entering a deeper C-state.
 *
 * Default: 20 % idle time.
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorIdlePromoteThresholdSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG Threshold;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid,
                          &GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    Threshold = PpmPolicyIdlePromoteThreshold;

    DPRINT("PopProcessorIdlePromoteThresholdSettingWorker: threshold=%lu%%\n",
           Threshold);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &Threshold,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Worker for GUID_PROCESSOR_CORE_PARKING_* scalar settings (min/max cores,
 * busy thresholds, hysteresis times).  Snapshots the current kernel policy
 * globals into registered driver callbacks and refreshes the park mask.
 *
 * Policy values are applied through @c PpmCoreParkingSetPolicyDword when
 * user-mode power policy code is wired; defaults live in park.c.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopProcessorCoreParkingSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG Value = 0;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    if (PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_PROCESSOR_CORE_PARKING_MAX_CORES))
        Value = PpmCoreParkingMaxCores;
    else if (PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_PROCESSOR_CORE_PARKING_MIN_CORES))
        Value = PpmCoreParkingMinCores;
    else if (PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD))
        Value = PpmCoreParkingBusyIncreaseThreshold;
    else if (PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD))
        Value = PpmCoreParkingBusyDecreaseThreshold;
    else if (PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME))
        Value = PpmCoreParkingIncreaseTime;
    else if (PopIsEqualGuid(&SettingCallback->SettingGuid, &GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME))
        Value = PpmCoreParkingDecreaseTime;

    PpmCoreParkingRefreshMask();

    DPRINT("PopProcessorCoreParkingSettingWorker: value=%lu\n", Value);

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &Value,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main worker thread for the GUID_SYSTEM_COOLING_POLICY power setting.
 * Updates the system-wide thermal cooling mode: passive (0) or active (1).
 *
 * Passive cooling throttles the processor; active cooling turns on fans.
 * The value is reflected in PopCoolingSystemMode which is consulted by the
 * thermal zone manager (thrmzn.c).
 *
 * @param[in] StartContext
 * Pointer to the registered power setting callback.
 */
_Function_class_(KSTART_ROUTINE)
static
VOID
NTAPI
PopSystemCoolingPolicySettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    ULONG CoolingMode;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);
    ASSERT(PopIsEqualGuid(&SettingCallback->SettingGuid,
                          &GUID_SYSTEM_COOLING_POLICY) == TRUE);

    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    /*
     * PopSystemCoolingMode: 0 = passive (throttle CPU), 1 = active (fans).
     * The thermal zone manager reads this global when deciding whether to
     * use passive or active cooling.  Snapshot the current value so it is
     * delivered to the registered callback.
     */
    CoolingMode = PopCoolingSystemMode;

    DPRINT("PopSystemCoolingPolicySettingWorker: mode=%lu (%s)\n",
           CoolingMode, CoolingMode ? "active" : "passive");

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &CoolingMode,
                                       sizeof(ULONG),
                                       SettingCallback->Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n",
                SettingCallback, Status);
    }

    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Main core worker thread of the power settings mechanism that deploys
 * the power setting workers for each registered callback for the specific
 * power setting GUID.
 *
 * @param[in] Parameter
 * A pointer to a parameter context. This points to the power setting GUID
 * that every power setting callback that has registered notifications for
 * this setting shall be notified.
 *
 * @remarks
 * A power setting callback might not be notified immediately or at all of
 * changes in a power setting for a couple of reasons:
 *
 * 1. The callback is dissappearing soon (understood by the POP_PSC_UNREGISTERED flag).
 * 2. The callback is currently executing the driver's supplied callback function
 *    (understood by the POP_PSC_ENTERING_CALLBACK flag).
 * 3. The callback is just about to be notified (understood by the POP_PSC_GETTING_NOTIFIED flag).
 *
 * The 3rd scenario usually happens when the Power Manager might notify the involved callbacks
 * multiple times in a row because of external factors that led to the specific power setting
 * data to be changed multiple times.
 *
 * An example of this would be when a human opens and closes a laptop's lid very fast and many times
 * continuously. Each time ACPI BIOS generates an OSPM notification of the lid being closed or opened,
 * the Power Manager receives this notification which in turn the power setting is changed multiples
 * times in a row. To avoid this, the Power Manager ensures that the callback is notified only
 * when it has previously finished its job first. This helps synchronizing the power setting notification
 * changes properly one at a time rather than deploying the said notifications all at once.
 */
_Use_decl_annotations_
static
VOID
NTAPI
PopDeployPowerSettingWorkers(
    _In_ PVOID Parameter)
{
    NTSTATUS Status;
    ULONG SettingIndex;
    GUID SettingGuid;
    PLIST_ENTRY ListHead, NextEntry;
    PPOP_POWER_SETTING_CALLBACK SettingCallback;
    PKSTART_ROUTINE PowerSettingWorker = NULL;
    PPOP_POWER_SETTING_NOTIFY_BLOCK NotifyBlock = (PPOP_POWER_SETTING_NOTIFY_BLOCK)Parameter;

    PAGED_CODE();

    ASSERT(NotifyBlock != NULL);

    /* Cache the setting GUID and free the notify block as we don't need it */
    RtlCopyMemory(&SettingGuid,
                  &NotifyBlock->NotifySettingGuid,
                  sizeof(GUID));
    PopFreePool(NotifyBlock, TAG_PO_POWER_SETTING_NOTIFY_BLOCK);

    /* Iterate over the list of registered setting callbacks and look for those we must notify */
    PopAcquirePowerSettingLock();
    ListHead = &PopPowerSettingCallbacksList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Check if this is the callback we must notify */
        SettingCallback = CONTAINING_RECORD(NextEntry, POP_POWER_SETTING_CALLBACK, Link);
        if (PopIsEqualGuid(&SettingCallback->SettingGuid, &SettingGuid))
        {
            /*
             * The following setting callback is marked for deletion.
             * In this case don't bother with it, go for the next setting callback.
             */
            if (PopPowerSettingReadAttribute(SettingCallback, POP_PSC_UNREGISTERED))
            {
                continue;
            }

            /*
             * It could be that the setting callback we are going to notify is already
             * executing the caller's supplied callback. We cannot submit a notification
             * to this callback until the power setting worker has finished processing
             * the caller's callback, so wait for its completion.
             */
            if (PopPowerSettingReadAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK) ||
                PopPowerSettingReadAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED))
            {
                /*
                 * Ensure the callback hasn't reached completion just right after we
                 * were exactly checking if the setting callback is currently executing
                 * the caller's supplied callback or not.
                 */
                ASSERT(KeReadStateEvent(&SettingCallback->CallbackReturned) == 0);

                /* Wait for the callback to finish its tasks */
                KeWaitForSingleObject(&SettingCallback->CallbackReturned,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
            }

            /* Mark this setting callback as it's getting notified soon */
            PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);

            /*
             * Retrieve the power setting worker to handle this callback and
             * submit it to the Power Manager to create a worker thread for it,
             * the kernel will execute the worker shortly.
             */
            for (SettingIndex = 0;
                 SettingIndex < POP_MAX_POWER_SETTINGS;
                 SettingIndex++)
            {
                if (PopIsEqualGuid(PopPowerSettingsDatabase[SettingIndex].SettingGuid, &SettingGuid))
                {
                    PowerSettingWorker = PopPowerSettingsDatabase[SettingIndex].SettingRoutine;
                    break;
                }
            }

            /*
             * FIXME: If a specific power setting doesn't come with a dedicated
             * setting worker then use the default worker which takes care of
             * unimplemented power settings. This is a placebo solution and
             * it must be treated as such. As every power setting is implemented
             * this workaround must be removed!
             */
            if (PowerSettingWorker == NULL)
            {
                PowerSettingWorker = PopUnimplementedPowerSettingWorker;
            }

            /*
             * Now service this power setting callback with a power manager
             * worker thread. We're putting it at a highest priority as the
             * driver who has registered for power setting notifications
             * must be notified as soon as possible at the time of a state
             * in change of a power setting.
             */
            Status = PopCreateWorkerThread(PowerSettingWorker,
                                           SettingCallback,
                                           POP_POWER_SETTING_WORKER_THREAD_PRIORITY);
            if (!NT_SUCCESS(Status))
            {
                /*
                 * Failing to service it with a power manager worker thread could
                 * lead to bugs like drivers never being notified of power setting
                 * changes. Unfabricate the system!
                 */
                KeBugCheckEx(INTERNAL_POWER_ERROR,
                             0,
                             POP_POWER_SETTING_CALLBACK_THREAD_CREATION_FAILURE,
                             (ULONG_PTR)&SettingGuid,
                             (ULONG_PTR)SettingCallback);
            }
        }
    }

    /* We're done deploying power setting workers */
    PopReleasePowerSettingLock();
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * The dummy worker thread used to invoke power setting callbacks for unimplemented
 * power settings. This is a placebo solution.
 *
 * @param[in] StartContext
 * A pointer to a start argument context, usually pointing to a setting callback
 * to be notified.
 */
_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopUnimplementedPowerSettingWorker(
    _In_ PVOID StartContext)
{
    NTSTATUS Status;
    UNICODE_STRING GuidString;
    ULONG DummyValue = 0;
    PPOP_POWER_SETTING_CALLBACK SettingCallback = (PPOP_POWER_SETTING_CALLBACK)StartContext;

    PAGED_CODE();

    ASSERT(SettingCallback != NULL);

    /* Display the power setting GUID for debugging purposes */
    Status = RtlStringFromGUID(&SettingCallback->SettingGuid, &GuidString);
    ASSERT(NT_SUCCESS(Status));
    DPRINT1("WARNING: Dummy worker called for power setting GUID, expect driver issues! (Guid: %wZ)\n", &GuidString);
    RtlFreeUnicodeString(&GuidString);

    /*
     * No callback must be executing at the time of calling this worker
     * to handle this power setting.
     */
    POP_ASSERT_NO_RUNNING_CALLBACK(SettingCallback);

    /*
     * This power setting is notified, acknowledge the Power Manager
     * that this power setting is entering into the driver's callback
     * to deliver the state change of the power setting.
     */
    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_GETTING_NOTIFIED);
    PopPowerSettingApplyAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);

    /* Execute the driver's supplied callback */
    Status = SettingCallback->Callback(&SettingCallback->SettingGuid,
                                       &DummyValue,
                                       sizeof(ULONG),
                                       SettingCallback->Context);

    /* Log to the debugger what happened with the callback */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The driver's callback of power setting 0x%p has failed (Status 0x%lx)\n", SettingCallback, Status);
    }

    /* Operation done, alert the Power Manager this power setting has finished its job */
    PopPowerSettingClearAttribute(SettingCallback, POP_PSC_ENTERING_CALLBACK);
    PopAlertCallbackReturned(SettingCallback);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * @brief
 * Creates a power setting callback.
 *
 * @param[in] DeviceObject
 * A pointer to a device object that wants to register a callback to receive
 * notifications of changes in a power setting. T
 *
 * @param[in] SettingGuid
 * A pointer to a power setting GUID that identifies the power setting in question.
 *
 * @param[in] Callback
 * A pointer to a driver's supplied callback. The Power Manager calls this callback
 * whenever the driver has to be notified.
 *
 * @param[in] Context
 * A pointer to a parameter context that is passed to the callback routine.
 * This parameter is optional.
 *
 * @param[out] PowerSettingCallback
 * A pointer to a power setting callback that's been created, returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the power setting callback has been created successfully.
 * STATUS_INSUFFICIENT_RESOURCES is returned if there's no enough memory to be
 * allocated for the power setting callback.
 */
NTSTATUS
NTAPI
PopAllocatePowerSettingCallback(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Out_ PPOP_POWER_SETTING_CALLBACK *PowerSettingCallback)
{
    PPOP_POWER_SETTING_CALLBACK SettingCallback;

    /* The calling thread must own the lock in order to register callbacks */
    POP_ASSERT_POWER_SETTING_LOCK_THREAD_OWNERSHIP();

    /* Allocate some memory pool for the power setting callback */
    SettingCallback = PopAllocatePool(sizeof(*SettingCallback),
                                      TRUE,
                                      TAG_PO_POWER_SETTING_CALLBACK);
    if (SettingCallback == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Setup the callback fields */
    SettingCallback->DeviceObject = DeviceObject;
    SettingCallback->Callback = Callback;
    SettingCallback->Context = Context;
    SettingCallback->Flags |= POP_PSC_REGISTERED;

    /* Initialize the link entry */
    InitializeListHead(&SettingCallback->Link);

    /* Copy the power setting GUID identifier */
    RtlCopyMemory(&SettingCallback->SettingGuid,
                  SettingGuid,
                  sizeof(GUID));

    /* Initialize the callback return event */
    KeInitializeEvent(&SettingCallback->CallbackReturned,
                      SynchronizationEvent,
                      FALSE);

    /* Give the allocated power setting callback to the caller */
    *PowerSettingCallback = SettingCallback;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Frees a power setting callback from memory.
 *
 * @param[in] PowerSettingCallback
 * A pointer to a power setting callback to be freed.
 */
VOID
NTAPI
PopReleasePowerSettingCallback(
    _In_ _Post_invalid_ PPOP_POWER_SETTING_CALLBACK PowerSettingCallback)
{
    /* The calling thread must own the lock in order to unregister callbacks */
    POP_ASSERT_POWER_SETTING_LOCK_THREAD_OWNERSHIP();

    /* Unlink the registered callback from the list */
    RemoveEntryList(&PowerSettingCallback->Link);

    /* And punt the callback from memory */
    PopFreePool(PowerSettingCallback, TAG_PO_POWER_SETTING_CALLBACK);
}

/**
 * @brief
 * Searches for the internal power setting callback structure by
 * hinting towards the driver's supplied callback function.
 *
 * @param[in] Callback
 * A pointer to a driver's supplied callback routine.
 *
 * @return
 * Returns a pointer to a power setting callback used for internal
 * purposes by the Power Manager. NULL is returned if nothing could be found.
 */
PPOP_POWER_SETTING_CALLBACK
NTAPI
PopFindPowerSettingCallbackByCallback(
    _In_ PPOWER_SETTING_CALLBACK Callback)
{
    PLIST_ENTRY ListHead, NextEntry;
    PPOP_POWER_SETTING_CALLBACK SettingCallback;

    /* The calling thread must own the lock in order to look inside the callbacks list */
    POP_ASSERT_POWER_SETTING_LOCK_THREAD_OWNERSHIP();

    /* Iterate over the registered power setting callbacks list and look for the right one */
    ListHead = &PopPowerSettingCallbacksList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Check if this is the callback we're looking for */
        SettingCallback = CONTAINING_RECORD(NextEntry, POP_POWER_SETTING_CALLBACK, Link);
        if (SettingCallback->Callback == Callback)
        {
            ASSERT((SettingCallback->Flags & POP_PSC_REGISTERED));
            return SettingCallback;
        }
    }

    /* Otherwise we haven't found anything we looked for */
    return NULL;
}

/**
 * @brief
 * Looks for the appropriate power setting worker routine to be used
 * to notify the callback.
 *
 * @param[in] SettingGuid
 * A pointer to a power setting GUID that identifies the said setting.
 *
 * @return
 * Returns a pointer to the power setting routine.
 */
PKSTART_ROUTINE
NTAPI
PopGetPowerSettingHelper(
    _In_ LPCGUID SettingGuid)
{
    ULONG SettingIndex;

    /* Iterate over the power settings database and look for the right setting */
    for (SettingIndex = 0;
         SettingIndex < POP_MAX_POWER_SETTINGS;
         SettingIndex++)
    {
        /* Is this what the caller asked for? */
        if (PopIsEqualGuid(PopPowerSettingsDatabase[SettingIndex].SettingGuid, SettingGuid))
        {
            return PopPowerSettingsDatabase[SettingIndex].SettingRoutine;
        }
    }

    return NULL;
}

/**
 * @brief
 * Notifies every power setting callback that has registered to receive
 * notifications for changes in a power setting pointed by the setting GUID.
 *
 * @param[in] SettingGuid
 * A pointer to a power setting GUID that identifies the said setting.
 * The master worker will notify the callbacks registered for this GUID.
 *
 * @remarks
 * The reason the master worker is handled by a Executive worker item thread
 * and not by creating a system thread just straight away is because we want
 * to notify changes of power setting(s) in all cases when we are in a higher
 * IRQL, DISPATCH_LEVEL respectively. Creating a system thread for the master
 * worker at DISPATCH_LEVEL is simply not possible.
 *
 * Cases for the Power Manager to enter in a state of being in DISPATCH_LEVEL
 * is not to be ruled out.
 */
VOID
NTAPI
PopNotifyPowerSettingChange(
    _In_ LPCGUID SettingGuid)
{
    PPOP_POWER_SETTING_NOTIFY_BLOCK NotifyBlock;

    /*
     * There's no active power setting callback occuring. It's pointless to
     * bother the master worker to deploy power setting workers.
     */
    if (PopPowerSettingCallbacksCount == 0)
    {
        ASSERT(IsListEmpty(&PopPowerSettingCallbacksList));
        return;
    }

    /*
     * The list is not empty, allocate a notify block and let the master
     * worker to deploy power setting workers specifically for this GUID.
     */
    NotifyBlock = PopAllocatePool(sizeof(*NotifyBlock),
                                  FALSE,
                                  TAG_PO_POWER_SETTING_NOTIFY_BLOCK);
    if (NotifyBlock == NULL)
    {
        /*
         * Failing to allocate a notification block for this setting GUID
         * could lead to bugs like power settings never being notified.
         * Unfabricate the system!
         */
        KeBugCheckEx(INTERNAL_POWER_ERROR,
                     0,
                     POP_POWER_SETTING_CALLBACK_DEPLOY_WORKERS_FAILURE,
                     (ULONG_PTR)SettingGuid,
                     0);
    }

    /* Copy the power setting GUID of which all the respective callbacks must be notified */
    RtlCopyMemory(&NotifyBlock->NotifySettingGuid,
                  SettingGuid,
                  sizeof(GUID));

    /*
     * Setup the notification item and invoke the master worker
     * to deploy power setting workers for this GUID.
     */
    ExInitializeWorkItem(&NotifyBlock->NotifyWorkItem,
                         PopDeployPowerSettingWorkers,
                         NotifyBlock);
    ExQueueWorkItem(&NotifyBlock->NotifyWorkItem, CriticalWorkQueue);
}

/* EOF */

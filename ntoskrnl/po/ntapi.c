/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager NT API system calls
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <internal/ppm.h>

/* GLOBALS ********************************************************************/

WORK_QUEUE_ITEM PopUnlockMemoryWorkItem;
KEVENT PopUnlockMemoryCompleteEvent;

/* In-memory power settings not stored in SYSTEM_POWER_POLICY (query/set round-trip). */
static ULONG PopPowerSettingMonitorOn = 1;
static ULONG PopPowerSettingConsoleDisplayState = 1;

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * A worker thread used to unlock memory that was previously locked for
 * hibernation purposes. When hibernation image preparation fails or is
 * aborted, any pages that were locked into the non-paged pool to prevent
 * them from being swapped out during the hibernation write phase must be
 * unlocked so that the memory manager can reclaim them.
 *
 * @param[in] Parameter
 * Unused context parameter (reserved for future use).
 */
_Use_decl_annotations_
VOID
NTAPI
PopUnlockMemoryWorker(
    _In_ PVOID Parameter)
{
    UNREFERENCED_PARAMETER(Parameter);

    /*
     * Unlock the pages that were locked for the hibernation image.
     * MmUnlockPagableImageSection releases any image sections that were
     * locked into non-paged pool during the hibernation preparation phase.
     *
     * FIXME: When full hibernation support is implemented, this worker should:
     *   1. Walk the locked memory list built during PopSaveHiberContext.
     *   2. Call MmUnlockPages for each MDL that was locked for hibernation I/O.
     *   3. Free any MDLs and associated bookkeeping structures.
     *   4. Signal PopUnlockMemoryCompleteEvent so that the calling thread in
     *      NtSetSystemPowerState knows it is safe to continue.
     */
    DPRINT("PopUnlockMemoryWorker: Signalling memory unlock completion\n");

    /* Signal that the memory unlock phase has completed */
    KeSetEvent(&PopUnlockMemoryCompleteEvent, IO_NO_INCREMENT, FALSE);
}

/**
 * @brief
 * Determines whether the executing calling thread has
 * enough privileges to perform certain power set information
 * operations.
 *
 * @param[in] Level
 * The power information level of which the calling thread
 * is setting information into.
 *
 * @param[in] InputBuffer
 * A pointer to an input buffer.
 *
 * @param[in] PreviousMode
 * The requestor mode of the calling thread.
 *
 * @return
 * Returns TRUE if the caller has enough privileges or if
 * the calling thread in question is called from kernel mode.
 * Otherwise FALSE is returned.
 */
static
BOOLEAN
PopIsCallerPrivileged(
    _In_ POWER_INFORMATION_LEVEL Level,
    _In_ PVOID InputBuffer,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    LUID Privilege;

    PAGED_CODE();

    /* If the caller was coming from the kernel, he has absolute privileges */
    if (PreviousMode == KernelMode)
    {
        return TRUE;
    }

    /* Setting a new power state handler can only be done in KM */
    if ((Level == SystemPowerStateHandler) && (PreviousMode != KernelMode))
    {
        return FALSE;
    }

    /*
     * This is coming from UM of which we cannot trust it, and we might have an
     * input buffer. Ensure that the caller has the appropriate privilege for
     * whatever operation he wants to do.
     */
    if (InputBuffer)
    {
        /*
         * The calling thread is merely asking the Power Manager to do some
         * verifications by providing their version of a power policy from
         * the input buffer. This is a vetting process and information is not
         * set onto the critical Power Manager data so the following information
         * classes are not invasive.
         */
        if (Level == VerifySystemPolicyAc ||
            Level == VerifySystemPolicyDc ||
            Level == VerifyProcessorPowerPolicyAc ||
            Level == VerifyProcessorPowerPolicyDc)
        {
            return TRUE;
        }

        /*
         * Query-like levels that still pass a small input buffer (group index or
         * setting GUID) must not require shutdown privilege.
         */
        if (Level == ProcessorInformationEx ||
            Level == GetPowerSettingValue)
        {
            return TRUE;
        }

        Privilege = (Level == SystemReserveHiberFile) ? SeCreatePagefilePrivilege : SeShutdownPrivilege;
        if (!SeSinglePrivilegeCheck(Privilege, PreviousMode))
        {
            return FALSE;
        }
    }

    /*
     * This is a query operation (understood by InputBuffer being NULL) or
     * the caller has the required privilege, allow him access.
     */
    return TRUE;
}

/**
 * @brief
 * Fills @a ProcessorCount entries with per-logical-processor power data
 * (same layout as Windows @c PopProcessorInformation for a single group).
 */
static
VOID
PopFillProcessorPowerInformationArray(
    _Out_writes_(ProcessorCount) PPROCESSOR_POWER_INFORMATION ProcInfo,
    _In_ ULONG ProcessorCount)
{
    ULONG i;

    for (i = 0; i < ProcessorCount; i++)
    {
        PKPRCB Prcb = KiProcessorBlock[i];
        PPPM_PERF_STATES_EX Perf;
        PPPM_IDLE_STATES_EX Idle;

        RtlZeroMemory(&ProcInfo[i], sizeof(ProcInfo[i]));
        ProcInfo[i].Number = i;

        if (Prcb == NULL)
            continue;

        Perf = (PPPM_PERF_STATES_EX)Prcb->PowerState.IdleHandlers;
        Idle = (PPPM_IDLE_STATES_EX)Prcb->PowerState.IdleState;

        if (Perf != NULL && Perf->Version == 1 && Perf->NominalFrequency != 0)
            ProcInfo[i].MaxMhz = Perf->NominalFrequency;

        if (ProcInfo[i].MaxMhz != 0)
        {
            ProcInfo[i].CurrentMhz =
                (ProcInfo[i].MaxMhz * (ULONG)Prcb->PowerState.CurrentThrottle) / 100UL;
            ProcInfo[i].MhzLimit =
                (ProcInfo[i].MaxMhz * (ULONG)Prcb->PowerState.ProcessorMaxThrottle) / 100UL;
        }

        if (Idle != NULL && Idle->Version == 1 && Idle->ProcessorIdleCount > 0)
            ProcInfo[i].MaxIdleState = Idle->ProcessorIdleCount - 1;

        ProcInfo[i].CurrentIdleState = 0;
    }
}

static
PSYSTEM_POWER_POLICY
PopPolicyStoreForPowerCondition(_In_ SYSTEM_POWER_CONDITION Condition)
{
    switch (Condition)
    {
        case PoAc:
            return &PopAcPowerPolicy;
        case PoDc:
        case PoHot:
            return &PopDcPowerPolicy;
        default:
            return NULL;
    }
}

static
NTSTATUS
PopQueryPowerSettingUlong(
    _In_ LPCGUID SettingGuid,
    _Out_ PULONG ValueOut)
{
    if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_THROTTLE_MAXIMUM))
    {
        *ValueOut = (ULONG)PpmPolicyMaxThrottle;
        return STATUS_SUCCESS;
    }

    if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_THROTTLE_MINIMUM))
    {
        *ValueOut = (ULONG)PpmPolicyMinThrottle;
        return STATUS_SUCCESS;
    }

    if (PopIsEqualGuid(SettingGuid, &GUID_SYSTEM_COOLING_POLICY))
    {
        *ValueOut = PopCoolingSystemMode;
        return STATUS_SUCCESS;
    }

    if (PopIsEqualGuid(SettingGuid, &GUID_MONITOR_POWER_ON))
    {
        *ValueOut = PopPowerSettingMonitorOn;
        return STATUS_SUCCESS;
    }

    if (PopIsEqualGuid(SettingGuid, &GUID_CONSOLE_DISPLAY_STATE))
    {
        *ValueOut = PopPowerSettingConsoleDisplayState;
        return STATUS_SUCCESS;
    }

    if (PopDefaultPowerPolicy == NULL)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    if (PopIsEqualGuid(SettingGuid, &GUID_VIDEO_POWERDOWN_TIMEOUT))
        *ValueOut = PopDefaultPowerPolicy->VideoTimeout;
    else if (PopIsEqualGuid(SettingGuid, &GUID_STANDBY_TIMEOUT))
        *ValueOut = PopDefaultPowerPolicy->IdleTimeout;
    else if (PopIsEqualGuid(SettingGuid, &GUID_DISK_POWERDOWN_TIMEOUT))
        *ValueOut = PopDefaultPowerPolicy->SpindownTimeout;
    else if (PopIsEqualGuid(SettingGuid, &GUID_HIBERNATE_TIMEOUT))
        *ValueOut = PopDefaultPowerPolicy->DozeS4Timeout;
    else
        return STATUS_OBJECT_NAME_NOT_FOUND;

    return STATUS_SUCCESS;
}

static
NTSTATUS
PopApplyPowerSettingUlong(
    _In_ LPCGUID SettingGuid,
    _In_opt_ PSYSTEM_POWER_POLICY TargetPolicy,
    _In_reads_bytes_(DataLength) PVOID Data,
    _In_ ULONG DataLength)
{
    ULONG Value;

    if (DataLength != sizeof(ULONG))
        return STATUS_INVALID_PARAMETER;

    Value = *(PULONG)Data;

    if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_THROTTLE_MAXIMUM))
    {
        if (Value > 100)
            Value = 100;
        PpmPolicyMaxThrottle = (UCHAR)Value;
        PopNotifyPowerSettingChange(&GUID_PROCESSOR_THROTTLE_MAXIMUM);
        return STATUS_SUCCESS;
    }

    if (PopIsEqualGuid(SettingGuid, &GUID_PROCESSOR_THROTTLE_MINIMUM))
    {
        if (Value > 100)
            Value = 100;
        if (TargetPolicy == NULL)
            return STATUS_INVALID_PARAMETER;
        TargetPolicy->MinThrottle = (UCHAR)Value;
        return STATUS_SUCCESS;
    }

    if (PopIsEqualGuid(SettingGuid, &GUID_SYSTEM_COOLING_POLICY))
    {
        PopCoolingSystemMode = Value ? 1UL : 0UL;
        PopNotifyPowerSettingChange(&GUID_SYSTEM_COOLING_POLICY);
        return STATUS_SUCCESS;
    }

    if (PopIsEqualGuid(SettingGuid, &GUID_MONITOR_POWER_ON))
    {
        PopPowerSettingMonitorOn = Value ? 1UL : 0UL;
        PopNotifyPowerSettingChange(&GUID_MONITOR_POWER_ON);
        return STATUS_SUCCESS;
    }

    if (PopIsEqualGuid(SettingGuid, &GUID_CONSOLE_DISPLAY_STATE))
    {
        PopPowerSettingConsoleDisplayState = Value;
        PopNotifyPowerSettingChange(&GUID_CONSOLE_DISPLAY_STATE);
        return STATUS_SUCCESS;
    }

    if (TargetPolicy == NULL)
        return STATUS_INVALID_PARAMETER;

    if (PopIsEqualGuid(SettingGuid, &GUID_VIDEO_POWERDOWN_TIMEOUT))
        TargetPolicy->VideoTimeout = Value;
    else if (PopIsEqualGuid(SettingGuid, &GUID_STANDBY_TIMEOUT))
        TargetPolicy->IdleTimeout = Value;
    else if (PopIsEqualGuid(SettingGuid, &GUID_DISK_POWERDOWN_TIMEOUT))
        TargetPolicy->SpindownTimeout = Value;
    else if (PopIsEqualGuid(SettingGuid, &GUID_HIBERNATE_TIMEOUT))
        TargetPolicy->DozeS4Timeout = Value;
    else
        return STATUS_OBJECT_NAME_NOT_FOUND;

    return STATUS_SUCCESS;
}

/* INFORMATION CLASSES ********************************************************/

static const INFORMATION_CLASS_INFO PoPowerInformationClass[] =
{
    /* SystemPowerPolicyAc */
    IQS_SAME(SYSTEM_POWER_POLICY, ULONG, ICIF_QUERY | ICIF_SET),

    /* SystemPowerPolicyDc */
    IQS_SAME(SYSTEM_POWER_POLICY, ULONG, ICIF_QUERY | ICIF_SET),

    /* VerifySystemPolicyAc */
    IQS_SAME(SYSTEM_POWER_POLICY, ULONG, ICIF_SET),

    /* VerifySystemPolicyDc */
    IQS_SAME(SYSTEM_POWER_POLICY, ULONG, ICIF_SET),

    /* SystemPowerCapabilities */
    IQS_SAME(SYSTEM_POWER_CAPABILITIES, ULONG, ICIF_QUERY),

    /* SystemBatteryState */
    IQS_SAME(SYSTEM_BATTERY_STATE, ULONG, ICIF_QUERY),

    /* SystemPowerStateHandler */
    IQS_SAME(POWER_STATE_HANDLER, ULONG, ICIF_SET),

    /* ProcessorStateHandler */
    IQS_SAME(PPM_DRIVER_DISPATCH_TABLE, ULONG, ICIF_QUERY),

    /* SystemPowerPolicyCurrent */
    IQS_SAME(SYSTEM_POWER_POLICY, ULONG, ICIF_QUERY),

    /* AdministratorPowerPolicy */
    IQS_SAME(ADMINISTRATOR_POWER_POLICY, ULONG, ICIF_QUERY | ICIF_SET),

    /* SystemReserveHiberFile */
    IQS_NONE,

    /* ProcessorInformation — one struct per logical processor (variable length) */
    IQS_SAME(PROCESSOR_POWER_INFORMATION, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),

    /* SystemPowerInformation */
    IQS_SAME(SYSTEM_POWER_INFORMATION, ULONG, ICIF_QUERY),

    /* ProcessorStateHandler2 */
    IQS_NONE,

    /* LastWakeTime */
    IQS_SAME(ULONG, ULONG, ICIF_QUERY),

    /* LastSleepTime */
    IQS_SAME(ULONG, ULONG, ICIF_QUERY),

    /* SystemExecutionState */
    IQS_SAME(ULONG, ULONG, ICIF_QUERY),

    /* SystemPowerStateNotifyHandler */
    IQS_NONE,

    /* ProcessorPowerPolicyAc */
    IQS_NONE,

    /* ProcessorPowerPolicyDc */
    IQS_NONE,

    /* VerifyProcessorPowerPolicyAc */
    IQS_NONE,

    /* VerifyProcessorPowerPolicyDc */
    IQS_NONE,

    /* ProcessorPowerPolicyCurrent */
    IQS_NONE,

    /* SystemPowerStateLogging */
    IQS_NONE,

    /* SystemPowerLoggingEntry */
    IQS_NONE,

    /* SetPowerSettingValue — SET_POWER_SETTING_VALUE + variable Data[] */
    IQS_NO_TYPE_LENGTH(ULONG, ICIF_SET | ICIF_SET_SIZE_VARIABLE),

    /* NotifyUserPowerSetting */
    IQS_NONE,

    /* PowerInformationLevelUnused0 */
    IQS_NONE, // Not used

    /* SystemMonitorHiberBootPowerOff */
    IQS_NONE,

    /* SystemVideoState */
    IQS_NONE,

    /* TraceApplicationPowerMessage */
    IQS_NONE,

    /* TraceApplicationPowerMessageEnd */
    IQS_NONE,

    /* ProcessorPerfStates */
    IQS_NONE,

    /* ProcessorIdleStates */
    IQS_NONE,

    /* ProcessorCap */
    IQS_NONE,

    /* SystemWakeSource */
    IQS_NONE,

    /* SystemHiberFileInformation */
    IQS_NONE,

    /* TraceServicePowerMessage */
    IQS_NONE,

    /* ProcessorLoad */
    IQS_NONE,

    /* PowerShutdownNotification */
    IQS_NONE,

    /* MonitorCapabilities */
    IQS_NONE,

    /* SessionPowerInit */
    IQS_NONE,

    /* SessionDisplayState */
    IQS_NONE,

    /* PowerRequestCreate */
    IQS_NONE,

    /* PowerRequestAction */
    IQS_NONE,

    /* GetPowerRequestList */
    IQS_NONE,

    /* ProcessorInformationEx */
    IQS_NONE,

    /* NotifyUserModeLegacyPowerEvent */
    IQS_NONE,

    /* GroupPark */
    IQS_NONE,

    /* ProcessorIdleDomains */
    IQS_NONE,

    /* WakeTimerList */
    IQS_NONE,

    /* SystemHiberFileSize */
    IQS_NONE,

    /* ProcessorIdleStatesHv */
    IQS_NONE,

    /* ProcessorPerfStatesHv */
    IQS_NONE,

    /* ProcessorPerfCapHv */
    IQS_NONE,

    /* ProcessorSetIdle */
    IQS_NONE,

    /* LogicalProcessorIdling */
    IQS_NONE,

    /* UserPresence */
    IQS_NONE,

    /* PowerSettingNotificationName */
    IQS_NONE,

    /* GetPowerSettingValue */
    IQS_NONE,

    /* IdleResiliency */
    IQS_NONE,

    /* SessionRITState */
    IQS_NONE,

    /* SessionConnectNotification */
    IQS_NONE,

    /* SessionPowerCleanup */
    IQS_NONE,

    /* SessionLockState */
    IQS_NONE,

    /* SystemHiberbootState */
    IQS_NONE,

    /* PlatformInformation */
    IQS_SAME(POWER_PLATFORM_INFORMATION, ULONG, ICIF_QUERY),

    /* PdcInvocation */
    IQS_NONE,

    /* MonitorInvocation */
    IQS_NONE,

    /* FirmwareTableInformationRegistered */
    IQS_NONE,

    /* SetShutdownSelectedTime */
    IQS_NONE,

    /* SuspendResumeInvocation */
    IQS_NONE,

    /* PlmPowerRequestCreate */
    IQS_NONE,

    /* ScreenOff */
    IQS_NONE,

    /* CsDeviceNotification */
    IQS_NONE,

    /* PlatformRole */
    IQS_NONE,

    /* LastResumePerformance */
    IQS_NONE,

    /* DisplayBurst */
    IQS_NONE,

    /* ExitLatencySamplingPercentage */
    IQS_NONE,

    /* RegisterSpmPowerSettings */
    IQS_NONE,

    /* PlatformIdleStates */
    IQS_NONE,

    /* ProcessorIdleVeto */
    IQS_NONE, // Deprecated

    /* PlatformIdleVeto */
    IQS_NONE, // Deprecated

    /* SystemBatteryStatePrecise */
    IQS_NONE,

    /* ThermalEvent */
    IQS_NONE,

    /* PowerRequestActionInternal */
    IQS_NONE,

    /* BatteryDeviceState */
    IQS_NONE,

    /* PowerInformationInternal */
    IQS_NONE,

    /* ThermalStandby */
    IQS_NONE,

    /* SystemHiberFileType */
    IQS_NONE,

    /* PhysicalPowerButtonPress */
    IQS_NONE,

    /* QueryPotentialDripsConstraint */
    IQS_NONE,

    /* EnergyTrackerCreate */
    IQS_NONE,

    /* EnergyTrackerQuery */
    IQS_NONE,

    /* UpdateBlackBoxRecorder */
    IQS_NONE,

    /* SessionAllowExternalDmaDevices */
    IQS_NONE,
};

/* SYSTEM CALLS ***************************************************************/

/**
 * @brief
 * Instantiates a global power action across the whole
 * system and devices. Devices, applications and everything else
 * are notified of this action.
 *
 * @param[in] SystemAction
 * Specifies the system action the Power Manager must take.
 *
 * @param[in] MinSystemState
 * Specifies the minimum system power state the system must take.
 * The Power Manager cannot let the system incur into the lowest
 * system state than what this parameter permits.
 *
 * @param[in] Flags
 * A bitmask flag passed by the caller that changes the behavior of
 * how this function must work. The following flags are:
 *
 * POWER_ACTION_CRITICAL -- The power action to be instantiated is
 *                          extremely critical and must be processed above
 *                          other registered power actions.
 *
 * POWER_ACTION_DISABLE_WAKES -- Tells the Power Mananger that with this power
 *                               action all wake events such as those from HAL
 *                               are disabled.
 *
 * POWER_ACTION_LIGHTEST_FIRST -- Tells this function to use the minimum system
 *                                sleep state provided by MinSystemState first.
 *                                The system action is thereby ignored.
 *
 * POWER_ACTION_LOCK_CONSOLE -- Tells the Power Manager that the user must input
 *                              a password upon resume of the system from a sleep
 *                              state due to this power action.
 *
 * POWER_ACTION_UI_ALLOWED -- Notifies all the applications of an impeding power
 *                            action that is occurring and the user is prompted
 *                            for directions on how to prepare for a change in state
 *                            of system power state like suspension.
 *
 * @param[in] Asynchronous
 * If set to TRUE, the power action operation is done asynchronously.
 * The Power Manager will deploy a DPC to handle the power action outside
 * of the current calling thread. The function will return immediately.
 * If set to FALSE, the following operation is synchronous and blocking,
 * as in, the calling thread will have to wait for the Power Manager until
 * it finishes processing the power action.
 */
NTSTATUS
NTAPI
NtInitiatePowerAction(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags,
    _In_ BOOLEAN Asynchronous)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    /* Capture the previous processor mode of the calling thread */
    PreviousMode = ExGetPreviousMode();

    /*
     * Warm eject is a privileged operation that only kernel callers are
     * permitted to issue directly. User-mode callers cannot trigger it via
     * NtInitiatePowerAction.
     */
    if (PreviousMode != KernelMode && SystemAction == PowerActionWarmEject)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* User-mode callers must hold the ShutdownPrivilege */
    if (PreviousMode != KernelMode)
    {
        if (!SeSinglePrivilegeCheck(SeShutdownPrivilege, PreviousMode))
        {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    /*
     * Validate the parameters. The minimum system state must be within the
     * legal range of defined SYSTEM_POWER_STATE values, the action must be
     * a recognised POWER_ACTION value, and no unknown flag bits may be set.
     *
     * Additionally, PowerActionSleep is not permitted to target the hibernate
     * state (PowerSystemHibernate) or deeper, and no server-silo threads may
     * initiate non-shutdown power actions.
     */
    if (MinSystemState > PowerSystemMaximum ||
        SystemAction > PowerActionWarmEject  ||
        (Flags & ~(POWER_ACTION_QUERY_ALLOWED  |
                   POWER_ACTION_UI_ALLOWED     |
                   POWER_ACTION_OVERRIDE_APPS  |
                   POWER_ACTION_LIGHTEST_FIRST |
                   POWER_ACTION_LOCK_CONSOLE   |
                   POWER_ACTION_DISABLE_WAKES  |
                   POWER_ACTION_CRITICAL))     ||
        (SystemAction == PowerActionSleep && MinSystemState >= PowerSystemHibernate))
    {
        DPRINT1("NtInitiatePowerAction: Invalid parameter combination "
                "(Action %u, MinState %u, Flags 0x%lx)\n",
                SystemAction, MinSystemState, Flags);
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Asynchronous power actions are dispatched by forwarding directly to
     * NtSetSystemPowerState, which will queue the graceful shutdown work item
     * and return immediately if not running in the system process context.
     *
     * Synchronous power actions are also forwarded to NtSetSystemPowerState
     * but the calling thread blocks until the action completes (or is vetoed).
     *
     * NOTE: The Windows kernel uses an internal PopExecutePowerAction path
     * with a POP_TRIGGER_WAIT structure for synchronous waits with a 15-second
     * timeout. For ReactOS the simpler NtSetSystemPowerState path is used
     * until the full Power Action Manager (PAM) infrastructure is in place.
     */
    DPRINT("NtInitiatePowerAction: Action %u, MinState %u, Flags 0x%lx, Async %u\n",
           SystemAction, MinSystemState, Flags, Asynchronous);

    Status = ZwSetSystemPowerState(SystemAction, MinSystemState, Flags);
    return Status;
}

/**
 * @brief
 * Queries information from the Power Manager or sets new information.
 *
 * @param[in] PowerInformationLevel
 * Specifies the power information level that is to be queried or set.
 *
 * @param[in] InputBuffer
 * A pointer to an input buffer of which information is used by the
 * Power Mananger to set a new information.
 *
 * @param[in] InputBufferLength
 * The length size of the buffer pointed by InputBuffer.
 *
 * @param[in] OutputBuffer
 * A pointer to an output buffer of which information is retrieved from
 * the Power Manager.
 *
 * @param[in] OutputBufferLength
 * The length size of the buffer pointed by OutputBuffer, this is so
 * the Power Manager understands how much of data is to be written into
 * the OutputBuffer parameter. Note that the buffer size can vary depending
 * on the power information level.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation has succeeded. STATUS_INVALID_PARAMETER
 * is returned if at least one of the parameters is not valid. STATUS_BUFFER_TOO_SMALL
 * is returned if the buffer pointed by OutputBuffer is too small to hold the
 * information. A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
NtPowerInformation(
    _In_ POWER_INFORMATION_LEVEL PowerInformationLevel,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    SYSTEM_BATTERY_STATE LocalBatteryState = {0};
    PSYSTEM_BATTERY_STATE BatteryState;
    POWER_PLATFORM_INFORMATION LocalPlatformInfo;
    PPOWER_PLATFORM_INFORMATION PlatformInfo;
    PSYSTEM_POWER_CAPABILITIES PowerCapabilities;
    PPOWER_STATE_HANDLER StateHandler;
    PSYSTEM_POWER_POLICY CurrentPolicy;
    PVOID LocalBuffer = NULL;

    PAGED_CODE();

#if DBG
    DPRINT("NtPowerInformation(PowerInformationLevel %S, InputBuffer 0x%p, "
           "InputBufferLength 0x%x, OutputBuffer 0x%p, OutputBufferLength 0x%x)\n",
           PopGetPowerInformationLevelName(PowerInformationLevel),
           InputBuffer, InputBufferLength,
           OutputBuffer, OutputBufferLength);
#endif

    /* Bail out if the caller is doing something special and has no required privilege */
    PreviousMode = ExGetPreviousMode();
    if (!PopIsCallerPrivileged(PowerInformationLevel, InputBuffer, PreviousMode))
    {
        DPRINT1("The caller has no required privilege for this operation, bail out\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /*
     * ProcessorInformationEx — input: processor group index (USHORT); output: array of
     * PROCESSOR_POWER_INFORMATION for that group.  Unlike most query levels, this uses
     * both buffers, so it is handled before the generic input-vs-output probe split.
     *
     * GetPowerSettingValue — input: 16-byte setting GUID; output: ULONG value (Windows
     * PopGetSettingValue contract for the settings we support).
     */
    if (PowerInformationLevel == ProcessorInformationEx ||
        PowerInformationLevel == GetPowerSettingValue)
    {
        if (PowerInformationLevel == ProcessorInformationEx)
        {
            USHORT GroupIndex;
            ULONG ProcessorCount, RequiredLength;

            if (!InputBuffer || InputBufferLength < sizeof(USHORT) ||
                !OutputBuffer || OutputBufferLength == 0)
            {
                return STATUS_INVALID_PARAMETER;
            }

            _SEH2_TRY
            {
                if (PreviousMode != KernelMode)
                    ProbeForRead(InputBuffer, sizeof(USHORT), sizeof(USHORT));
                RtlCopyMemory(&GroupIndex, InputBuffer, sizeof(USHORT));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                return _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /*
             * ReactOS does not expose multiple processor groups yet; only group 0 is valid.
             */
            if (GroupIndex != 0)
                return STATUS_INVALID_PARAMETER;

            ProcessorCount = (ULONG)KeNumberProcessors;
            RequiredLength = sizeof(PROCESSOR_POWER_INFORMATION) * ProcessorCount;
            if (OutputBufferLength < RequiredLength)
                return STATUS_BUFFER_TOO_SMALL;

            _SEH2_TRY
            {
                if (PreviousMode != KernelMode)
                    ProbeForWrite(OutputBuffer, RequiredLength, sizeof(ULONG));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                return _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            PopAcquirePowerPolicyLock();
            PopFillProcessorPowerInformationArray(
                (PPROCESSOR_POWER_INFORMATION)OutputBuffer,
                ProcessorCount);
            PopReleasePowerPolicyLock();
            return STATUS_SUCCESS;
        }

        /* GetPowerSettingValue */
        {
            GUID SettingGuid;
            ULONG Value;

            if (!InputBuffer || InputBufferLength < sizeof(GUID) ||
                !OutputBuffer || OutputBufferLength < sizeof(ULONG))
            {
                return STATUS_INVALID_PARAMETER;
            }

            _SEH2_TRY
            {
                if (PreviousMode != KernelMode)
                    ProbeForRead(InputBuffer, sizeof(GUID), sizeof(ULONG));
                RtlCopyMemory(&SettingGuid, InputBuffer, sizeof(GUID));

                ProbeForWrite(OutputBuffer, sizeof(ULONG), sizeof(ULONG));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                return _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            PopAcquirePowerPolicyLock();
            Status = PopQueryPowerSettingUlong(&SettingGuid, &Value);
            PopReleasePowerPolicyLock();

            if (!NT_SUCCESS(Status))
                return Status;

            _SEH2_TRY
            {
                *(PULONG)OutputBuffer = Value;
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            return Status;
        }
    }

    /* Probe the parameters depending on whether the caller queries or sets something */
    if (InputBuffer)
    {
        /* It wants to set new power data, make sure the length was provided */
        ASSERT(InputBufferLength > 0);

        /* And probe the actual buffer data and class validity */
        Status = DefaultSetInfoBufferCheck(PowerInformationLevel,
                                           PoPowerInformationClass,
                                           RTL_NUMBER_OF(PoPowerInformationClass),
                                           InputBuffer,
                                           InputBufferLength,
                                           PreviousMode);
        if (NT_SUCCESS(Status))
        {
            if (PreviousMode == UserMode)
            {
                /*
                 * We are not done here. We must allocate a local buffer to hold the
                 * input data so that we are safe from anybody who frees InputBuffer.
                 */
                LocalBuffer = PopAllocatePool(InputBufferLength,
                                              TRUE,
                                              TAG_PO_INPUT_INFO_CLASS_BUFFER);
                if (LocalBuffer == NULL)
                {
                    DPRINT1("Buffer allocation for input data failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    /* Copy the power input data now */
                    RtlCopyMemory(LocalBuffer, InputBuffer, InputBufferLength);
                }
            }
            else
            {
                /*
                 * The caller is coming from the kernel. We do not need to
                 * allocate any buffer because we trust the kernel.
                 */
                LocalBuffer = InputBuffer;
            }
        }
    }
    else
    {
        /* The caller wants to query something from the Power Manager */
        ASSERT(OutputBufferLength > 0);

        /* Probe the parameter and class validity */
        Status = DefaultQueryInfoBufferCheck(PowerInformationLevel,
                                             PoPowerInformationClass,
                                             RTL_NUMBER_OF(PoPowerInformationClass),
                                             ICIF_PROBE_READ_WRITE,
                                             OutputBuffer,
                                             OutputBufferLength,
                                             NULL,
                                             NULL,
                                             PreviousMode);
    }

    /* Probing failed, bail out */
    if (!NT_SUCCESS(Status))
    {
#if DBG
        DPRINT1("Information verification class failed (Status -> 0x%lx, PowerInformationLevel -> %S)\n",
                Status, PopGetPowerInformationLevelName(PowerInformationLevel));
#endif
        return Status;
    }

    /* Do the Query/Set power operations with the policy lock held */
    PopAcquirePowerPolicyLock();

    switch (PowerInformationLevel)
    {
        case SystemPowerPolicyAc:
        case SystemPowerPolicyDc:
        {
            PSYSTEM_POWER_POLICY TargetPolicy;
            PSYSTEM_POWER_POLICY PolicyBuffer;
            BOOLEAN IsAc = (PowerInformationLevel == SystemPowerPolicyAc);

            TargetPolicy = IsAc ? &PopAcPowerPolicy : &PopDcPowerPolicy;

            if (InputBuffer)
            {
                /*
                 * The caller is setting a new AC or DC power policy. Validate
                 * the revision and then overwrite the existing policy.
                 */
                PolicyBuffer = (PSYSTEM_POWER_POLICY)LocalBuffer;
                if (PolicyBuffer->Revision != POP_SYSTEM_POWER_POLICY_REVISION_V1)
                {
                    DPRINT1("SystemPowerPolicyAc/Dc: Invalid policy revision %lu\n",
                            PolicyBuffer->Revision);
                    Status = STATUS_INVALID_PARAMETER;
                    goto Quit;
                }

                _SEH2_TRY
                {
                    RtlCopyMemory(TargetPolicy, PolicyBuffer, sizeof(SYSTEM_POWER_POLICY));
                    Status = STATUS_SUCCESS;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                if (!NT_SUCCESS(Status))
                {
                    goto Quit;
                }

                /*
                 * Notify Win32k and power setting callbacks of the policy change.
                 * PopPowerPolicyNotification broadcasts PsW32CapabilitiesChanged
                 * and PsW32PowerPolicyChanged so that all subsystems re-read the
                 * active policy and update their behaviour (e.g. display timeouts,
                 * idle timers).
                 */
                PopPowerPolicyNotification();

                /*
                 * Propagate processor-specific fields (e.g. MinThrottle) from
                 * the newly installed policy into the kernel PPM engine globals
                 * and notify any GUID_PROCESSOR_THROTTLE_MINIMUM callbacks.
                 */
                PopSyncPpmPolicyFromCurrentPolicy();

                /*
                 * Schedule the system-idle worker so that display and idle
                 * timeouts from the newly applied policy are immediately honoured.
                 */
                PopRequestPolicyWorker(PolicyWorkerSystemIdle);
                PopCheckForPendingWorkers();
            }
            else
            {
                /* The caller is querying the current AC or DC power policy */
                if (OutputBufferLength < sizeof(SYSTEM_POWER_POLICY))
                {
                    DPRINT1("SystemPowerPolicyAc/Dc: OutputBufferLength too small (%lu)\n",
                            OutputBufferLength);
                    Status = STATUS_BUFFER_TOO_SMALL;
                    goto Quit;
                }

                _SEH2_TRY
                {
                    RtlCopyMemory(OutputBuffer, TargetPolicy, sizeof(SYSTEM_POWER_POLICY));
                    Status = STATUS_SUCCESS;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            break;
        }

        case VerifySystemPolicyAc:
        case VerifySystemPolicyDc:
        {
            PSYSTEM_POWER_POLICY TargetPolicy;
            PSYSTEM_POWER_POLICY PolicyBuffer;
            BOOLEAN IsAc = (PowerInformationLevel == VerifySystemPolicyAc);

            /*
             * These are Set-only information levels. The caller submits a
             * SYSTEM_POWER_POLICY that the Power Manager validates and
             * potentially clamps to the system's capabilities. On success
             * the validated policy is written back to the AC or DC store so
             * that the caller can read it back via SystemPowerPolicyAc/Dc.
             */
            if (!InputBuffer || !LocalBuffer)
            {
                DPRINT1("VerifySystemPolicyAc/Dc: InputBuffer is required\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            PolicyBuffer = (PSYSTEM_POWER_POLICY)LocalBuffer;
            TargetPolicy = IsAc ? &PopAcPowerPolicy : &PopDcPowerPolicy;

            /* Only revision 1 is supported */
            if (PolicyBuffer->Revision != POP_SYSTEM_POWER_POLICY_REVISION_V1)
            {
                DPRINT1("VerifySystemPolicyAc/Dc: Invalid policy revision %lu\n",
                        PolicyBuffer->Revision);
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /*
             * Clamp MinSleep/MaxSleep to valid system sleep states. Windows
             * enforces PowerSystemSleeping1 as the minimum and
             * PowerSystemHibernate as the maximum. If the policy requests
             * a state the hardware does not support, it is silently promoted
             * to the next supported state via PopCapabilities.SystemS*.
             */
            if (PolicyBuffer->MinSleep < PowerSystemSleeping1)
            {
                PolicyBuffer->MinSleep = PowerSystemSleeping1;
            }
            if (PolicyBuffer->MaxSleep < PolicyBuffer->MinSleep ||
                PolicyBuffer->MaxSleep > PowerSystemHibernate)
            {
                PolicyBuffer->MaxSleep = PowerSystemHibernate;
            }

            /* Persist the validated policy */
            _SEH2_TRY
            {
                RtlCopyMemory(TargetPolicy, PolicyBuffer, sizeof(SYSTEM_POWER_POLICY));
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            if (NT_SUCCESS(Status))
            {
                /*
                 * Mirror SystemPowerPolicyAc/Dc: validated policy is persisted into
                 * the AC/DC store; refresh Win32k, push MinThrottle (and related
                 * notifications) into PPM, and reschedule idle policy work.
                 */
                PopPowerPolicyNotification();
                PopSyncPpmPolicyFromCurrentPolicy();
                PopRequestPolicyWorker(PolicyWorkerSystemIdle);
                PopCheckForPendingWorkers();
            }
            break;
        }

        case AdministratorPowerPolicy:
        {
            PADMINISTRATOR_POWER_POLICY AdminPolicyBuffer;

            if (InputBuffer)
            {
                /*
                 * The caller is updating the administrative power policy overrides.
                 * These override per-user policy settings (e.g. minimum/maximum
                 * sleep states, video and spindown timeout ranges) to enforce
                 * enterprise power management policy.
                 */
                if (InputBufferLength < sizeof(ADMINISTRATOR_POWER_POLICY))
                {
                    DPRINT1("AdministratorPowerPolicy: InputBufferLength too small (%lu)\n",
                            InputBufferLength);
                    Status = STATUS_BUFFER_TOO_SMALL;
                    goto Quit;
                }

                AdminPolicyBuffer = (PADMINISTRATOR_POWER_POLICY)LocalBuffer;

                _SEH2_TRY
                {
                    RtlCopyMemory(&PopAdminPowerPolicy,
                                  AdminPolicyBuffer,
                                  sizeof(ADMINISTRATOR_POWER_POLICY));
                    Status = STATUS_SUCCESS;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            else
            {
                /* The caller is querying the current administrative power policy */
                if (OutputBufferLength < sizeof(ADMINISTRATOR_POWER_POLICY))
                {
                    DPRINT1("AdministratorPowerPolicy: OutputBufferLength too small (%lu)\n",
                            OutputBufferLength);
                    Status = STATUS_BUFFER_TOO_SMALL;
                    goto Quit;
                }

                _SEH2_TRY
                {
                    RtlCopyMemory(OutputBuffer,
                                  &PopAdminPowerPolicy,
                                  sizeof(ADMINISTRATOR_POWER_POLICY));
                    Status = STATUS_SUCCESS;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            break;
        }

        case SystemPowerCapabilities:
        {
            PowerCapabilities = (PSYSTEM_POWER_CAPABILITIES)OutputBuffer;

            /* The caller provided an input buffer on a Query class, bail out */
            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on SystemPowerCapabilities class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right output buffer length, bail out */
            if (OutputBufferLength < sizeof(SYSTEM_POWER_CAPABILITIES))
            {
                DPRINT1("OutputBufferLength too small (length %lu)\n", OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* FIXME: We should filter the capabilities if the system has legacy stuff */
            _SEH2_TRY
            {
                RtlCopyMemory(PowerCapabilities,
                              &PopCapabilities,
                              sizeof(SYSTEM_POWER_CAPABILITIES));

                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case SystemBatteryState:
        {
            BatteryState = (PSYSTEM_BATTERY_STATE)OutputBuffer;

            /* The caller provided an input buffer on a Query class, bail out */
            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on SystemBatteryState class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right output buffer length, bail out */
            if (OutputBufferLength < sizeof(SYSTEM_BATTERY_STATE))
            {
                DPRINT1("OutputBufferLength too small (length %lu)\n", OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* Query the current state of the composite battery */
            PopQueryBatteryState(&LocalBatteryState);

            /* Copy the current state of the composite battery to the caller */
            _SEH2_TRY
            {
                RtlCopyMemory(BatteryState,
                              &LocalBatteryState,
                              sizeof(LocalBatteryState));

                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case ProcessorInformation:
        {
            PPROCESSOR_POWER_INFORMATION ProcInfo;
            ULONG ProcessorCount;
            ULONG RequiredLength;

            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on ProcessorInformation when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            ProcessorCount = (ULONG)KeNumberProcessors;
            RequiredLength = sizeof(PROCESSOR_POWER_INFORMATION) * ProcessorCount;
            if (OutputBufferLength < RequiredLength)
            {
                DPRINT1("ProcessorInformation: need %lu bytes, got %lu\n",
                        RequiredLength, OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            ProcInfo = (PPROCESSOR_POWER_INFORMATION)OutputBuffer;
            PopFillProcessorPowerInformationArray(ProcInfo, ProcessorCount);

            Status = STATUS_SUCCESS;
            break;
        }

        case SystemPowerInformation:
        {
            PSYSTEM_POWER_INFORMATION SysPwrInfo;

            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on SystemPowerInformation when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            if (OutputBufferLength < sizeof(SYSTEM_POWER_INFORMATION))
            {
                DPRINT1("SystemPowerInformation: OutputBufferLength too small (%lu)\n",
                        OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            SysPwrInfo = (PSYSTEM_POWER_INFORMATION)OutputBuffer;
            RtlZeroMemory(SysPwrInfo, sizeof(*SysPwrInfo));

            if (PopDefaultPowerPolicy != NULL)
            {
                SysPwrInfo->MaxIdlenessAllowed = PopDefaultPowerPolicy->IdleTimeout;
                SysPwrInfo->TimeRemaining = PopDefaultPowerPolicy->IdleTimeout;
            }

            SysPwrInfo->CoolingMode = (UCHAR)PopCoolingSystemMode;

            Status = STATUS_SUCCESS;
            break;
        }

        case LastWakeTime:
        case LastSleepTime:
        {
            PULONG TimeValue = (PULONG)OutputBuffer;

            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on LastWakeTime/LastSleepTime when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            if (OutputBufferLength < sizeof(ULONG))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /*
             * ReactOS does not yet maintain high-resolution wake/sleep timestamps
             * in the Power Manager; return zero (unknown / not tracked).
             */
            _SEH2_TRY
            {
                *TimeValue = 0;
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case SystemExecutionState:
        {
            PEXECUTION_STATE EsOut = (PEXECUTION_STATE)OutputBuffer;

            if (InputBuffer)
            {
                DPRINT1("Setting SystemExecutionState via NtPowerInformation is not supported\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            if (OutputBufferLength < sizeof(EXECUTION_STATE))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            _SEH2_TRY
            {
                *EsOut = PopQueryAggregateLegacyExecutionState();
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case PlatformInformation:
        {
            PlatformInfo = (PPOWER_PLATFORM_INFORMATION)OutputBuffer;

            /* The caller provided an input buffer on a Query class, bail out */
            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on PlatformInformation class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right output buffer length, bail out */
            if (OutputBufferLength < sizeof(POWER_PLATFORM_INFORMATION))
            {
                DPRINT1("OutputBufferLength too small (length %lu)\n", OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* Determine the AoAc capability from the global Power Manager variable */
            LocalPlatformInfo.AoAc = PopAoAcPresent;

            _SEH2_TRY
            {
                RtlCopyMemory(PlatformInfo,
                              &LocalPlatformInfo,
                              sizeof(POWER_PLATFORM_INFORMATION));

                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case SystemPowerStateHandler:
        {
            /* The caller asked for an output buffer on a Set class, bail out */
            if (OutputBuffer)
            {
                DPRINT1("OutputBuffer provided on SystemPowerStateHandler class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right input buffer length, bail out */
            if (InputBufferLength < sizeof(POWER_STATE_HANDLER))
            {
                DPRINT1("InputBufferLength too small (length %lu)\n", InputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* No input buffer was provided on a Set class, bail out */
            if (!LocalBuffer)
            {
                DPRINT1("No InputBuffer was provided for the SystemPowerStateHandler class\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /*
             * Setting system power state handlers is ACPI turf, the system must be
             * already supporting ACPI when we're being given state handlers. APM
             * and non-ACPI systems cannot use this class so assert this condition.
             */
            ASSERT(PopAcpiPresent == TRUE);

            /* Cache the provided newer state handler */
            StateHandler = (PPOWER_STATE_HANDLER)LocalBuffer;

            /*
             * HALs can only register newer power state handlers only once and
             * not more. Check that we already have a state handler registered
             * of the specified type. We allow HALs to replace our default
             * power off handler with theirs.
             */
            if (StateHandler->Type == PowerStateShutdownOff)
            {
                if (PopDefaultPowerStateHandlers[StateHandler->Type].Handler != PopShutdownHandler)
                {
                    DPRINT1("The HAL already replaced the default power Off state handler, cannot replace it twice\n");
                    Status = STATUS_INVALID_PARAMETER;
                    goto Quit;
                }
            }
            else
            {
                if (PopDefaultPowerStateHandlers[StateHandler->Type].Handler != NULL)
                {
                    DPRINT1("Cannot set new state handler twice for type (%ld)\n", StateHandler->Type);
                    Status = STATUS_INVALID_PARAMETER;
                    goto Quit;
                }
            }

            /* Register a new state handler with the Power Manager */
            Status = PopRegisterSystemStateHandler(StateHandler->Type,
                                                   StateHandler->RtcWake,
                                                   StateHandler->Handler,
                                                   StateHandler->Context);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to register a state handler for type %ld (Status 0x%lx)\n", StateHandler->Type, Status);
                goto Quit;
            }

            /*
             * Registration done successfully. Now we must activate the following
             * system state capability based on the state handler the HAL has given to us.
             */
            PopChangeSystemSystemStateCapability(StateHandler, TRUE);
            break;
        }

        case ProcessorStateHandler:
        {
            /*
             * ZwPowerInformation(ProcessorStateHandler, NULL, 0, &Table, sizeof(Table))
             *
             * Called by processor drivers (intelppm, amdppm) during initialization to
             * obtain the kernel's PPM driver dispatch table.  The driver uses the table
             * to register P-states, C-states, and performance caps with the OS power
             * manager.
             *
             * The kernel does not accept a SET for this level (input buffer is ignored);
             * only a GET is meaningful.
             */

            /* No input buffer expected for this query */
            if (InputBuffer)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            if (OutputBufferLength < sizeof(PPM_DRIVER_DISPATCH_TABLE))
            {
                DPRINT1("ProcessorStateHandler: OutputBufferLength %lu < %lu\n",
                        OutputBufferLength, (ULONG)sizeof(PPM_DRIVER_DISPATCH_TABLE));
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            _SEH2_TRY
            {
                RtlCopyMemory(OutputBuffer,
                              &PpmKernelDispatchTable,
                              sizeof(PPM_DRIVER_DISPATCH_TABLE));
                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case SystemPowerPolicyCurrent:
        {
            CurrentPolicy = (PSYSTEM_POWER_POLICY)OutputBuffer;

            /* The caller provided an input buffer on a Query class, bail out */
            if (InputBuffer)
            {
                DPRINT1("InputBuffer provided on SystemPowerPolicyCurrent class when it should not be\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            /* This is not the right output buffer length, bail out */
            if (OutputBufferLength < sizeof(SYSTEM_POWER_POLICY))
            {
                DPRINT1("OutputBufferLength too small (length %lu)\n", OutputBufferLength);
                Status = STATUS_BUFFER_TOO_SMALL;
                goto Quit;
            }

            /* Copy the currently enacted system policy to the caller */
            _SEH2_TRY
            {
                RtlCopyMemory(CurrentPolicy,
                              PopDefaultPowerPolicy,
                              sizeof(SYSTEM_POWER_POLICY));

                Status = STATUS_SUCCESS;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case SetPowerSettingValue:
        {
            PSET_POWER_SETTING_VALUE SetVal;
            ULONG MinimumSize;
            PSYSTEM_POWER_POLICY TargetPolicy;
            BOOLEAN NotifyPolicy;

            if (!LocalBuffer || OutputBuffer != NULL)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            SetVal = (PSET_POWER_SETTING_VALUE)LocalBuffer;

            if (SetVal->Version != POWER_SETTING_VALUE_VERSION)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            if (SetVal->PowerCondition >= PoConditionMaximum || SetVal->DataLength == 0)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            MinimumSize = FIELD_OFFSET(SET_POWER_SETTING_VALUE, Data) + SetVal->DataLength;
            if (InputBufferLength < MinimumSize)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }

            NotifyPolicy = FALSE;

            if (PopIsEqualGuid(&SetVal->Guid, &GUID_PROCESSOR_THROTTLE_MAXIMUM))
            {
                Status = PopApplyPowerSettingUlong(&SetVal->Guid,
                                                   NULL,
                                                   SetVal->Data,
                                                   SetVal->DataLength);
            }
            else if (PopIsEqualGuid(&SetVal->Guid, &GUID_SYSTEM_COOLING_POLICY))
            {
                Status = PopApplyPowerSettingUlong(&SetVal->Guid,
                                                   NULL,
                                                   SetVal->Data,
                                                   SetVal->DataLength);
            }
            else if (PopIsEqualGuid(&SetVal->Guid, &GUID_MONITOR_POWER_ON) ||
                     PopIsEqualGuid(&SetVal->Guid, &GUID_CONSOLE_DISPLAY_STATE))
            {
                Status = PopApplyPowerSettingUlong(&SetVal->Guid,
                                                   NULL,
                                                   SetVal->Data,
                                                   SetVal->DataLength);
            }
            else
            {
                TargetPolicy = PopPolicyStoreForPowerCondition(SetVal->PowerCondition);
                if (TargetPolicy == NULL)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    goto Quit;
                }

                Status = PopApplyPowerSettingUlong(&SetVal->Guid,
                                                   TargetPolicy,
                                                   SetVal->Data,
                                                   SetVal->DataLength);

                if (NT_SUCCESS(Status) &&
                    PopIsEqualGuid(&SetVal->Guid, &GUID_PROCESSOR_THROTTLE_MINIMUM) &&
                    PopDefaultPowerPolicy == TargetPolicy)
                {
                    NotifyPolicy = TRUE;
                }
                else if (NT_SUCCESS(Status) &&
                         (PopIsEqualGuid(&SetVal->Guid, &GUID_VIDEO_POWERDOWN_TIMEOUT) ||
                          PopIsEqualGuid(&SetVal->Guid, &GUID_STANDBY_TIMEOUT) ||
                          PopIsEqualGuid(&SetVal->Guid, &GUID_DISK_POWERDOWN_TIMEOUT) ||
                          PopIsEqualGuid(&SetVal->Guid, &GUID_HIBERNATE_TIMEOUT)))
                {
                    NotifyPolicy = (PopDefaultPowerPolicy == TargetPolicy);
                }
            }

            if (NT_SUCCESS(Status) &&
                (NotifyPolicy ||
                 PopIsEqualGuid(&SetVal->Guid, &GUID_PROCESSOR_THROTTLE_MAXIMUM) ||
                 PopIsEqualGuid(&SetVal->Guid, &GUID_SYSTEM_COOLING_POLICY) ||
                 PopIsEqualGuid(&SetVal->Guid, &GUID_MONITOR_POWER_ON) ||
                 PopIsEqualGuid(&SetVal->Guid, &GUID_CONSOLE_DISPLAY_STATE)))
            {
                PopPowerPolicyNotification();
                PopSyncPpmPolicyFromCurrentPolicy();
                PopRequestPolicyWorker(PolicyWorkerSystemIdle);
                PopCheckForPendingWorkers();
            }

            break;
        }

        default:
        {
#if DBG
            DPRINT1("%S information class is UNIMPLEMENTED\n", PopGetPowerInformationLevelName(PowerInformationLevel));
#endif
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

Quit:
    PopReleasePowerPolicyLock();

    if (LocalBuffer && PreviousMode == UserMode)
    {
        PopFreePool(LocalBuffer, TAG_PO_INPUT_INFO_CLASS_BUFFER);
    }

    return Status;
}

/**
 * @brief
 * Retrieves the power state of a device.
 *
 * @param[in] Device
 * A handle to a device.
 *
 * @param[out] PowerState
 * A pointer to a power state of the device in question,
 * returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation has succeeded.
 * A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
NtGetDevicePowerState(
    _In_ HANDLE Device,
    _Out_ PDEVICE_POWER_STATE PowerState)
{
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    KPROCESSOR_MODE PreviousMode;
    PDEVICE_OBJECT DeviceObject;
    DEVICE_POWER_STATE DeviceState;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExts;

    PAGED_CODE();

    /* The caller must supply a variable to the output argument */
    ASSERT(PowerState);

    /* Ensure that we can return the device power state to the caller */
    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteUlong(PowerState);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Reference the device so that we can get its device extensions */
    Status = ObReferenceObjectByHandle(Device,
                                       0,
                                       IoFileObjectType,
                                       PreviousMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference the device (Status 0x%lx)\n", Status);
        return Status;
    }

    /*
     * Get the device object and put a reference on it so that it does not
     * die in our arms. We no longer care about the file object.
     */
    DeviceObject = IoGetRelatedDeviceObject(FileObject);
    ObReferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);

    /* Get the extensions and retrieve the power state of this device */
    DevObjExts = IoGetDevObjExtension(DeviceObject);
    DeviceState = PopGetDoePowerState(DevObjExts, FALSE);

    /* Return the current power state to the caller safely */
    _SEH2_TRY
    {
        *PowerState = DeviceState;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ObDereferenceObject(DeviceObject);
    return Status;
}

/**
 * @brief
 * Determines whether the system was resumed automatically
 * without from a human prompt (like a wake up timeout signal)
 * or not.
 *
 * @return
 * Returns TRUE if the system was resumed automatically without
 * human input, FALSE otherwise.
 */
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(VOID)
{
    /* Tell the caller whether the system resumed automatically from the Power Manager */
    return PopResumeAutomatic;
}

/**
 * @brief
 * Tells the Power Manager to respect a certain wakeup latency
 * offset when the system wakes up. This makes the Power Manager
 * to choose a low power state that doesn't take much time for
 * the system to wake up that could exceed the said offset limit.
 *
 * @param[in] Latency
 * The wakeup latency time requirement. The following values are:
 *
 * LT_LOWEST_LATENCY -- The lowest latency possible. This means
 *                      the Power Manager will choose a state of
 *                      PowerSystemSleeping1 (aka S1) which is
 *                      sleep-to-RAM and the system is slightly
 *                      powered down.
 *
 * LT_DONT_CARE -- Any latency is permitted, the Power Manager will
 *                 take whatever sleep state when a power action
 *                 is to be taken.
 *
 * @remarks
 * This function is obsolete and meant to exist only for compatibility
 * purposes.
 */
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    _In_ LATENCY_TIME Latency)
{
    /* On Windows Vista and later versions of Windows, this function is a NOP */
    UNREFERENCED_PARAMETER(Latency);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Makes the system busy due to certain activity the current
 * calling thread is currently doing.
 *
 * @param[in] esFlags
 * A bitmask flag provided by the caller. See PoRegisterSystemState for
 * further information about the flags.
 *
 * @param[out] PreviousFlags
 * A pointer to the previous execution state flags of the calling
 * thread, returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation has succeeded. A failure
 * NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    _In_ EXECUTION_STATE esFlags,
    _Out_ EXECUTION_STATE *PreviousFlags)
{
    NTSTATUS Status;
    PETHREAD Thread;
    EXECUTION_STATE PreviousState;
    PPOP_POWER_REQUEST PowerRequest;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    /* The caller passed ES_USER_PRESENT when it's not even supported (see MSDN documentation) */
    if (esFlags & ~(ES_CONTINUOUS | ES_USER_PRESENT))
    {
        DPRINT1("ES_USER_PRESENT is not supported when setting a new thread execution state\n");
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Probe the output parameter so that we are safe that we can return
     * the previous execution state of the calling thread.
     */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteUlong(PreviousFlags);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Cache the current calling thread and check if it has a power request */
    Thread = PsGetCurrentThread();
    if (Thread->LegacyPowerObject == NULL)
    {
        /*
         * This thread never had a power request object so that means this is
         * the first time it actually sets an execution power state. Create a
         * power request but the legacy one, as we are dealing with legacy
         * execution state flags here.
         */
        Status = PopRegisterPowerRequest(NULL,
                                         RegisterLegacyRequest,
                                         TRUE,
                                         esFlags,
                                         NULL,
                                         &PowerRequest);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to create a legacy power request object for thread 0x%p (Status 0x%lx)\n", Thread, Status);
            return Status;
        }

        /*
         * As this is the first time this thread has a power request, setup
         * the state flags to an initial state, typically 0.
         */
        Thread->LegacyPowerObject = (PVOID)Thread->LegacyPowerObject;
        PreviousState = 0UL | ES_CONTINUOUS;
        goto Exit;
    }

    /* Cache the previous execution state flags before changing them with newer ones */
    PowerRequest = (PPOP_POWER_REQUEST)Thread->LegacyPowerObject;
    PreviousState = PowerRequest->LegacyStateFlags;

    /*
     * This thread already has a power request, the register helper will
     * take care of changing its execution state flags.
     */
    Status = PopRegisterPowerRequest(NULL,
                                     RegisterLegacyRequest,
                                     FALSE,
                                     esFlags,
                                     NULL,
                                     &PowerRequest);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to change execution state flags for thread 0x%p and power request 0x%p (Status 0x%lx)\n",
                Thread, Thread->LegacyPowerObject, Status);
        return Status;
    }

Exit:
    _SEH2_TRY
    {
        *PreviousFlags = PreviousState;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return Status;
}

/**
 * @brief
 * Sets a new power state to the system, thereby powering it
 * down.
 *
 * @param[in] SystemAction
 * Specifies the system action the Power Manager must take.
 *
 * @param[in] MinSystemState
 * Specifies the minimum system power state the system must take.
 * The Power Manager cannot let the system incur into the lowest
 * system state than what this parameter permits.
 *
 * @param[in] Flags
 * A bitmask flag passed by the caller that changes the behavior of
 * how this function must work. For further details, see NtInitiatePowerAction.
 *
 * @return
 * Returns STATUS_SYSTEM_SHUTDOWN to indicate the system is shutting
 * down soon. STATUS_PRIVILEGE_NOT_HELD is returned if the shutdown privilege
 * is not held by the current calling thread to invoke any change to the
 * system power state. STATUS_PRIVILEGE_NOT_HELD is returned if one
 * of the parameters is not valid. A failure NTSTATUS code is returned
 * otherwise.
 */
NTSTATUS
NTAPI
NtSetSystemPowerState(
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags)
{
    NTSTATUS Status;
    ULONG FreedPagesCount;
    KPROCESSOR_MODE PreviousMode;
    POP_POWER_ACTION Action;

    /* Look for any invalid argument and bail out */
    if ((MinSystemState >= PowerSystemMaximum) ||
        (MinSystemState <= PowerSystemUnspecified) ||
        (SystemAction > PowerActionWarmEject) ||
        (SystemAction < PowerActionReserved) ||
        (Flags & ~(POWER_ACTION_QUERY_ALLOWED  |
                   POWER_ACTION_UI_ALLOWED     |
                   POWER_ACTION_OVERRIDE_APPS  |
                   POWER_ACTION_LIGHTEST_FIRST |
                   POWER_ACTION_LOCK_CONSOLE   |
                   POWER_ACTION_DISABLE_WAKES  |
                   POWER_ACTION_CRITICAL)))
    {
        DPRINT1("Invalid parameters found:\n");
        DPRINT1("   SystemAction: 0x%x\n", SystemAction);
        DPRINT1("   MinSystemState: 0x%x\n", MinSystemState);
        DPRINT1("   Flags: 0x%x\n", Flags);
        return STATUS_INVALID_PARAMETER;
    }

    /* Is this called from user mode? */
    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        /* Make sure that the caller has the required shutdown privilege */
        if (!SeSinglePrivilegeCheck(SeShutdownPrivilege, PreviousMode))
        {
            DPRINT1("Privilege not held for setting a system power state\n");
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        /* Turn this execution into kernel as this is an invasive operation */
        return ZwSetSystemPowerState(SystemAction, MinSystemState, Flags);
    }

    /* Disable lazy registry flushing */
    CmSetLazyFlushState(FALSE);

    /* Setup the power action */
    RtlZeroMemory(&Action, sizeof(Action));
    Action.Action = SystemAction;
    Action.Flags = Flags;

    /* Notify any registered callbacks of an impeding system state change */
    ExNotifyCallback(PowerStateCallback, (PVOID)PO_CB_SYSTEM_STATE_LOCK, NULL);

    /* Do not allow swaping worker threads at this operation */
    ExSwapinWorkerThreads(FALSE);

    /* Lock the entire power policy manager and make our action global */
    PopAcquirePowerPolicyLock();
    PopAction = Action;

    /* Process action requests */
    Status = STATUS_CANCELLED;
    while (TRUE)
    {
        /* No power action was inquired */
        if (Action.Action == PowerActionNone)
        {
            break;
        }

        /* Check if this action is of shutdown type */
        if (Status == STATUS_CANCELLED)
        {
            if ((PopAction.Action == PowerActionShutdown) ||
                (PopAction.Action == PowerActionShutdownReset) ||
                (PopAction.Action == PowerActionShutdownOff))
            {
                PopAction.Shutdown = TRUE;
            }

            Status = STATUS_SUCCESS;
        }

        /* Stop processin action requests if we are at an invalid status */
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* Flush all volumes and the registry */
        PopFlushVolumes(PopAction.Shutdown);

        /* Flush dirty cache pages */
        CcRosFlushDirtyPages(MAXULONG, &FreedPagesCount, FALSE, FALSE);

        /* Now execute the graceful shutdown */
        PopAction.IrpMinor = IRP_MN_SET_POWER;
        if (PopAction.Shutdown)
        {
            /* If we are not running in the system context then queue the shutdown worker */
            if (PsGetCurrentProcess() != PsInitialSystemProcess)
            {
                ExInitializeWorkItem(&PopShutdownWorkItem,
                                     &PopGracefulShutdown,
                                     NULL);
                ExQueueWorkItem(&PopShutdownWorkItem, CriticalWorkQueue);

                KeSuspendThread(KeGetCurrentThread());
                Status = STATUS_SYSTEM_SHUTDOWN;
                goto Exit;
            }
            else
            {
                /* We are running within the system context, invoke shutdown directly */
                PopGracefulShutdown(NULL);
            }
        }
        else if (PopAction.Action == PowerActionSleep ||
                 PopAction.Action == PowerActionHibernate)
        {
            SYSTEM_POWER_STATE TargetState;
            POWER_STATE PState;
            NTSTATUS IrpStatus;

            if (PopAction.Action == PowerActionHibernate)
            {
                TargetState = PowerSystemHibernate;
            }
            else
            {
                if (MinSystemState <= PowerSystemUnspecified ||
                    MinSystemState >= PowerSystemMaximum)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                if (MinSystemState == PowerSystemWorking)
                    TargetState = PowerSystemSleeping3;
                else
                    TargetState = MinSystemState;
            }

            if (PopSystemPowerDeviceNode == NULL ||
                PopSystemPowerDeviceNode->PhysicalDeviceObject == NULL)
            {
                DPRINT1("No registered system power device node; cannot broadcast sleep IRP\n");
                Status = STATUS_DEVICE_DOES_NOT_EXIST;
                break;
            }

            PopAction.SystemState = TargetState;
            PopAction.EffectiveSystemState = TargetState;
            PopAction.CurrentSystemState = PowerSystemWorking;
            PopAction.NextSystemState = TargetState;

            PState.SystemState = TargetState;
            IrpStatus = PopRequestSystemPowerIrp(PopSystemPowerDeviceNode->PhysicalDeviceObject,
                                                 IRP_MN_SET_POWER,
                                                 PState,
                                                 FALSE,
                                                 FALSE,
                                                 NULL,
                                                 NULL,
                                                 NULL);
            if (!NT_SUCCESS(IrpStatus) && IrpStatus != STATUS_PENDING)
                Status = IrpStatus;
            else
                Status = STATUS_SUCCESS;
            break;
        }

        /* There is A LOOOOOOOT OF STUFF TO IMPLEMENT HERE, consider it a stub at the moment */
        DPRINT1("System is still up and running, you may not have chosen a yet supported power option: %u\n", PopAction.Action);
        break;
    }

Exit:
    /* Release the policy manager lock and enable lazy registry flushing */
    PopReleasePowerPolicyLock();
    CmSetLazyFlushState(TRUE);
    return Status;
}

/**
 * @brief
 * Tells the Power Manager that a device requests the
 * system to wake up.
 *
 * @param[in] DeviceHandle
 * A handle to a device that is requesting a wakeup.
 */
NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    _In_ HANDLE DeviceHandle)
{
    /*
     * This function was designed to allow a device to request a system wakeup
     * but was never implemented in any version of Windows. The proper mechanism
     * for device wakeup is through IRP_MN_WAIT_WAKE power IRPs dispatched via
     * PoRequestPowerIrp. Drivers that previously called this function should
     * use the Power IRP infrastructure instead.
     */
    UNREFERENCED_PARAMETER(DeviceHandle);
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Cancels a wakeup request previously inquired by a device
 * with a call to NtRequestDeviceWakeup.
 *
 * @param[in] DeviceHandle
 * A handle to a device that requested a wakeup of the system.
 */
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    _In_ HANDLE DeviceHandle)
{
    /*
     * This function was designed as the counterpart to NtRequestDeviceWakeup
     * but was never implemented in any version of Windows. See the remarks
     * in NtRequestDeviceWakeup for the proper wakeup mechanism.
     */
    UNREFERENCED_PARAMETER(DeviceHandle);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

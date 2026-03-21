/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Policy Manager & Policy Worker Manager
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG PopShutdownPowerOffPolicy;
ERESOURCE PopPowerPolicyLock;
LIST_ENTRY PopPowerPolicyIrpQueueList;
WORK_QUEUE_ITEM PopPowerPolicyWorkItem;
PKTHREAD PopPowerPolicyOwnerLockThread;
KSPIN_LOCK PopPowerPolicyWorkerLock;
BOOLEAN PopPendingPolicyWorker;
SYSTEM_POWER_POLICY PopAcPowerPolicy;
SYSTEM_POWER_POLICY PopDcPowerPolicy;
PSYSTEM_POWER_POLICY PopDefaultPowerPolicy;
PKWIN32_POWEREVENT_CALLOUT PopEventCallout = NULL;
POP_POLICY_WORKER PopPolicyWorker[PolicyWorkerMax] = {0};

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Reads the shutdown power policy from the registry and caches
 * its value to a global variable.
 */
static
VOID
PopReadShutdownPolicy(VOID)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    ULONG Length;
    UNICODE_STRING KeyString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION Info = (PVOID)Buffer;

    /* Setup object attributes */
    RtlInitUnicodeString(&KeyString,
                         L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\Windows NT");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyString,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Default the policy value to 0 just in case if opening the key fails */
    PopShutdownPowerOffPolicy = 0;

    /* Open the key */
    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Open the policy value and query it */
        RtlInitUnicodeString(&KeyString, L"DontPowerOffAfterShutdown");
        Status = ZwQueryValueKey(KeyHandle,
                                 &KeyString,
                                 KeyValuePartialInformation,
                                 &Info,
                                 sizeof(Info),
                                 &Length);
        if ((NT_SUCCESS(Status)) && (Info->Type == REG_DWORD))
        {
            /* Read the policy */
            PopShutdownPowerOffPolicy = *Info->Data == 1;
        }

        /* Close the key */
        ZwClose(KeyHandle);
    }
}

/**
 * @brief
 * Removes a power policy device. The Power Policy Manager (PPM)
 * is notified of this when such a device is no longer present
 * in the system.
 *
 * @param[in] NotificationStructure
 * A pointer to a notification structure sent by the I/O manager
 * that comes with information about the device being removed.
 *
 * @param[in] PolicyDeviceType
 * A power policy device type, that represents the power policy
 * device being removed.
 *
 * @return
 * Returns STATUS_SUCCESS when a power policy device has been
 * successfully removed. STATUS_NOT_SUPPORTED is returned if
 * the Power Manager currently doesn't support removals for
 * the specific power policy device. STATUS_INSUFFICIENT_RESOURCES
 * is returned if there's no enough memory to be allocated for
 * a policy work item. A failure NTSTATUS code is returned otherwise.
 *
 * @remarks
 * Each power policy management module (such as thermal zones, composite
 * battery, control switches, et al) is responsible to perform any
 * cleanup or specific procedures in order to fully disconnect removed
 * policy devices from Power Manager.
 */
static
NTSTATUS
PopRemovePolicyDevice(
    _In_ PDEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationStructure,
    _In_ POWER_POLICY_DEVICE_TYPE PolicyDeviceType)
{
    NTSTATUS Status;
    PVOID PolicyData;
    BOOLEAN MustDereference = TRUE;
    PDEVICE_OBJECT PolicyDeviceObject;
    PWORKER_THREAD_ROUTINE PolicyWorkerRoutine;
    PPOP_DEVICE_POLICY_WORKITEM_DATA PolicyWorkItemData;
    PPOP_CONTROL_SWITCH ControlSwitch;

    PAGED_CODE();

    /* The power policy lock must be owned */
    POP_ASSERT_POWER_POLICY_LOCK_OWNERSHIP();

    /*
     * We received notification of the removal of the composite battery. There is
     * no need to reference the object twice as the battery device driver is always
     * the same and it does not change.
     */
    if (PolicyDeviceType == PolicyDeviceBattery)
    {
        /*
         * Of course the Power Manager must not be played by absolute fools that a
         * battery removal is happening when a battery was not even used in the first place...
         */
        ASSERT(!(PopBattery->Flags & POP_CB_NO_BATTERY));

        /* Cache the battery device object that we referenced before */
        PolicyDeviceObject = PopBattery->DeviceObject;

        /* The CB handler will dereference the battery device by itself, no need to do it twice */
        MustDereference = FALSE;
    }
    else
    {
        /*
         * We are opening a device that has already been opened with a reference but
         * on device removal we are going to de-reference the extra reference we held
         * (and on the policy device handler it will totally de-reference the last count)
         * so it should not be a problem anyway.
         */
        Status = PopGetPolicyDeviceObject(NotificationStructure->SymbolicLinkName,
                                          &PolicyDeviceObject);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open the device for removal\n");
            return Status;
        }
    }

    /* Allocate a policy device workitem */
    PolicyWorkItemData = PopAllocatePool(sizeof(POP_DEVICE_POLICY_WORKITEM_DATA),
                                         FALSE,
                                         TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
    if (PolicyWorkItemData == NULL)
    {
        DPRINT1("Failed to allocate pool of memory for the policy device workitem data\n");
        ObDereferenceObject(PolicyDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (PolicyDeviceType == PolicyDeviceSystemButton)
    {
        /* Grab the target control switch that we are going to remove */
        ControlSwitch = PopGetControlSwitchByDevice(PolicyDeviceObject);
        ASSERT(ControlSwitch != NULL);
        ControlSwitch->Flags |= POP_CS_CLEANUP;
        PolicyData = ControlSwitch;
        PolicyWorkerRoutine = PopControlSwitchHandler;
    }
    else if (PolicyDeviceType == PolicyDeviceMemory)
    {
        DPRINT1("Policy memory device not currently implemented yet\n");
        ASSERT(FALSE);
    }
    else if (PolicyDeviceType == PolicyDeviceBattery)
    {
        PopBattery->Flags |= POP_CB_REMOVE_BATTERY;
        PolicyData = NULL;
        PolicyWorkerRoutine = PopCompositeBatteryHandler;
    }
    else if (PolicyDeviceType == PolicyDeviceThermalZone)
    {
        DPRINT1("Policy thermal zone device not currently implemented yet\n");
        ASSERT(FALSE);
    }
    else if (PolicyDeviceType == PolicyDeviceFan)
    {
        DPRINT1("Policy fan device not currently implemented yet\n");
        ASSERT(FALSE);
    }
    else
    {
        /* We do not support device removals for unknown devices */
        DPRINT1("Got an unsupported policy device (%lu), expect weird system behaviors\n", PolicyDeviceType);
        PopFreePool(PolicyWorkItemData, TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
        ObDereferenceObject(PolicyDeviceObject);
        return STATUS_NOT_SUPPORTED;
    }

    /* Queue the appropriate policy device handler to remove this device */
    PolicyWorkItemData->PolicyData = PolicyData;
    PolicyWorkItemData->PolicyType = PolicyDeviceType;
    ExInitializeWorkItem(&PolicyWorkItemData->WorkItem,
                         PolicyWorkerRoutine,
                         PolicyWorkItemData);
    ExQueueWorkItem(&PolicyWorkItemData->WorkItem, DelayedWorkQueue);

    if (MustDereference)
    {
        ObDereferenceObject(PolicyDeviceObject);
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Connects a power policy device with the Power Policy Manager (PPM)
 * upon the arrival of said device, notified by the ACPI driver.
 *
 * @param[in] NotificationStructure
 * A pointer to a notification structure sent by the I/O manager
 * that comes with information about the device being added.
 *
 * @param[in] PolicyDeviceType
 * A power policy device type, that represents the power policy
 * device being added.
 *
 * @return
 * Returns STATUS_SUCCESS when a power policy device has been
 * successfully added. STATUS_NOT_SUPPORTED is returned if
 * the Power Manager currently doesn't support connections for
 * the specific power policy device. STATUS_INSUFFICIENT_RESOURCES
 * is returned if there's no enough memory to be allocated for
 * a policy work item. A failure NTSTATUS code is returned otherwise.
 */
static
NTSTATUS
PopAddPolicyDevice(
    _In_ PDEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationStructure,
    _In_ POWER_POLICY_DEVICE_TYPE PolicyDeviceType)
{
    NTSTATUS Status;
    PVOID PolicyData;
    PDEVICE_OBJECT PolicyDeviceObject;
    PWORKER_THREAD_ROUTINE PolicyWorkerRoutine;
    PPOP_DEVICE_POLICY_WORKITEM_DATA PolicyWorkItemData;
    PPOP_CONTROL_SWITCH ControlSwitch;
    UNICODE_STRING CompositeBatteryDeviceName;

    PAGED_CODE();

    /* The power policy lock must be owned */
    POP_ASSERT_POWER_POLICY_LOCK_OWNERSHIP();

    DPRINT1("Connecting with policy device %wZ\n", NotificationStructure->SymbolicLinkName);

    /*
     * This is an upcoming composite battery. There is only one instance of a
     * composite battery driver that takes care of all batteries present in a system.
     * With that said, if we get to this point and there is already a battery in use
     * then we have gotten a candiate of a new battery. The tag must be recalculated
     * (the Composite Battery driver will do that). There is no need to open the same
     * device object twice. Let the battery handler get acknowledged of a new battery
     * taking place.
     */
    if (PolicyDeviceType == PolicyDeviceBattery)
    {
        if (!(PopBattery->Flags & POP_CB_NO_BATTERY) ||
            PopCbConected == TRUE)
        {
            PopMarkNewBatteryPending(NotificationStructure->SymbolicLinkName);
            return STATUS_SUCCESS;
        }

        /*
         * The composite battery was never connected with the Power Manager,
         * it is time to do this now.
         */
        RtlInitUnicodeString(&CompositeBatteryDeviceName, L"\\Device\\CompositeBattery");

        /* Connect with the composite battery now */
        Status = PopGetPolicyDeviceObject(&CompositeBatteryDeviceName,
                                          &PolicyDeviceObject);
    }
    else
    {
        /* This is not a battery, connect with this policy device */
        Status = PopGetPolicyDeviceObject(NotificationStructure->SymbolicLinkName,
                                          &PolicyDeviceObject);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to connect with the policy device\n");
        return Status;
    }

    /* Allocate a policy device workitem, we will use it later below */
    PolicyWorkItemData = PopAllocatePool(sizeof(POP_DEVICE_POLICY_WORKITEM_DATA),
                                         FALSE,
                                         TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
    if (PolicyWorkItemData == NULL)
    {
        DPRINT1("Failed to allocate pool of memory for the policy device workitem data\n");
        ObDereferenceObject(PolicyDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Check what kind of policy device it is and initialize critical
     * datum with the power manager.
     */
    switch (PolicyDeviceType)
    {
        case PolicyDeviceSystemButton:
        {
            /* This is a control switch, let the CS manager setup data for it */
            Status = PopCreateControlSwitch(PolicyDeviceObject, &ControlSwitch);
            PolicyData = ControlSwitch;
            PolicyWorkerRoutine = PopControlSwitchHandler;
            break;
        }

        case PolicyDeviceBattery:
        {
            /*
             * There is an upcoming composite battery for the first time,
             * let the CB manager handle it.
             */
            Status = PopConnectCompositeBattery(PolicyDeviceObject);
            PolicyData = NULL;
            PolicyWorkerRoutine = PopCompositeBatteryHandler;
            break;
        }

        case PolicyDeviceThermalZone:
        {
            DPRINT1("Policy thermal zone device not currently implemented yet\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        case PolicyDeviceMemory:
        {
            DPRINT1("Policy memory device not currently implemented yet\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        case PolicyDeviceFan:
        {
            DPRINT1("Policy fan device not currently implemented yet\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }

        default:
        {
            /*
             * We got an unknown policy device of which we do not have any
             * infrastructure for it currently implemented. As a consequence
             * the system may not work properly due to foreign devices not
             * attached with the power manager.
             */
            DPRINT1("Got an unsupported policy device (%lu), expect weird system behaviors\n", PolicyDeviceType);
            PopFreePool(PolicyWorkItemData, TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
            ObDereferenceObject(PolicyDeviceObject);
            return STATUS_NOT_SUPPORTED;
        }
    }

    /* Bail out if the creation of policy device data has failed */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create data for the policy device (Status 0x%08lx, policy device %lu)\n", Status, PolicyDeviceType);
        PopFreePool(PolicyWorkItemData, TAG_PO_POLICY_DEVICE_WORKITEM_DATA);
        ObDereferenceObject(PolicyDeviceObject);
        return Status;
    }

    /*
     * We are almost done, fill in device policy data into the allocated workitem
     * so that this little cute boy gets dispatched to the appropriate policy device
     * handler routine.
     */
    PolicyWorkItemData->PolicyData = PolicyData;
    PolicyWorkItemData->PolicyType = PolicyDeviceType;
    ExInitializeWorkItem(&PolicyWorkItemData->WorkItem,
                         PolicyWorkerRoutine,
                         PolicyWorkItemData);

    /* Dispatch the policy device now */
    ExQueueWorkItem(&PolicyWorkItemData->WorkItem, DelayedWorkQueue);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Dispatches a power event to Win32k, the Win32 kernel-mode subsystem.
 * Win32k handles this power event and further deploys to all the
 * applications as well as Win32 related system components of the
 * occurring power event.
 *
 * @param[in] Win32PwrEventParams
 * A pointer to a set of parameters for the event that Win32k should
 * process.
 */
VOID
PopDispatchPowerEvent(
    _In_ PWIN32_POWEREVENT_PARAMETERS Win32PwrEventParams)
{
    PVOID Session;
    ULONG SessionId;
    ULONG ProcessFlags;
    KAPC_STATE ApcState;
    NTSTATUS Status;

    /* If we get here and the power callout was not registered yet, just quit */
    if (!PopEventCallout)
    {
        return;
    }

    /* Ensure the power callout is within the session space in order to be usable */
    /* FIXME: The event callout is never within the session space, this has to be investigated why */
#if 0
    ASSERT(MmIsSessionAddress(PopEventCallout) == TRUE);
#endif

    /*
     * FIXME: Unfortunately ReactOS currently does not support multiple
     * sessions. As a consequence we have to dispatch the power event
     * to only the active single session. This is not a problem per se
     * but when ReactOS will ever get multi-sessions support, this code
     * needs adaptations as it currently only dispatches power events to
     * only single session. For GDI events (PsW32GdiOn && PsW32GdiOff), these
     * shall be dispatched to the console session only, everything else have
     * to be broadcasted to all sessions.
     */

    /* Dispatch the power event directly if the process is within the session space */
    ProcessFlags = PsGetCurrentProcess()->Flags;
    SessionId = PsGetCurrentProcessSessionId();
    if (ProcessFlags & PSF_PROCESS_IN_SESSION_BIT)
    {
        PopEventCallout(Win32PwrEventParams);
        return;
    }

    /* It is not within the session space, get the session */
    Session = MmGetSessionById(SessionId);
    if (!Session)
    {
        /*
         * We cannot dispatch the power event to a session that does not
         * even exist. This would probably mean the process is within
         * a bogus session that was never inserted, so alert the debugger
         * in such case.
         */
        ASSERT(Session != NULL);
        return;
    }

    /* Attach to that session in order to dispatch the power event */
    Status = MmAttachSession(Session, &ApcState);
    if (!NT_SUCCESS(Status))
    {
        /* Failing to attach signifies a major problem, bail out */
        ASSERT(Status == STATUS_SUCCESS);
        return;
    }

    /* Dispatch the power event */
    PopEventCallout(Win32PwrEventParams);
    MmDetachSession(Session, &ApcState);
    MmQuitNextSession(Session);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Retrieves the device object to a power policy device.
 *
 * @param[in] DeviceName
 * A pointer to a Unicode string that points to the name
 * of the power policy device.
 *
 * @param[out] DeviceObject
 * A pointer to a device object that is returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the device object was retrieved
 * successfully. Otherwise a failure NSTATUS code is returned.
 */
NTSTATUS
NTAPI
PopGetPolicyDeviceObject(
    _In_ PUNICODE_STRING DeviceName,
    _Out_ PDEVICE_OBJECT *DeviceObject)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    PFILE_OBJECT FileObject;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    /* Open this policy device */
    InitializeObjectAttributes(&ObjectAttributes,
                               DeviceName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&FileHandle,
                        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open the policy device (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Reference the policy device and get a DO of it */
    Status = ObReferenceObjectByHandle(FileHandle,
                                       SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                                       IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference the policy device file object (Status 0x%08lx)\n", Status);
        ZwClose(FileHandle);
        return Status;
    }

    /*
     * Put a reference on the returned device object so that we can
     * safely discard the file object of which we no longer care about it.
     */
    *DeviceObject = IoGetRelatedDeviceObject(FileObject);
    ObReferenceObject(*DeviceObject);
    ObDereferenceObject(FileObject);
    ZwClose(FileHandle);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * A power policy function that's called whenever the power capabilities
 * or the current power policy have changed. Win32k and any registered
 * power callouts are notified of the new capabilities so that any
 * dependent subsystems can adapt their behavior accordingly.
 */
VOID
NTAPI
PopPowerPolicyNotification(VOID)
{
    WIN32_POWEREVENT_PARAMETERS Params;

    PAGED_CODE();

    /*
     * Notify Win32k that the power capabilities of the system have
     * changed. Win32k will update its internal state to reflect the
     * new capabilities (e.g. battery presence, sleep states, etc.).
     */
    Params.EventNumber = PsW32CapabilitiesChanged;
    Params.Code = 0;
    PopDispatchPowerEvent(&Params);

    /*
     * Also notify Win32k that the power policy itself has changed so that
     * any power-aware components running within the Win32 subsystem can
     * re-query the active policy and update their own behavior.
     */
    Params.EventNumber = PsW32PowerPolicyChanged;
    Params.Code = 0;
    PopDispatchPowerEvent(&Params);
}

/**
 * @brief
 * A power policy function that's called whenever the system starts idling.
 * The Power Manager uses this to evaluate whether the system has been idle
 * long enough to trigger a configured power action (e.g. sleep, hibernate).
 * Registered idle-state callbacks and Win32k are notified as appropriate.
 *
 * @remarks
 * This routine performs two duties:
 *
 * 1. Re-arms Win32k's video/display idle timers with the values carried in
 *    the currently active power policy (PopDefaultPowerPolicy->VideoTimeout
 *    and IdleTimeout). Win32k itself tracks elapsed inactivity time; when
 *    the video timeout fires, Win32k sends PsW32GdiOffRequest to darken the
 *    display. When the idle timeout fires, Win32k notifies the Power Manager
 *    via the power callout mechanism, which eventually re-triggers this worker.
 *
 * 2. If a non-None idle power action is configured
 *    (PopDefaultPowerPolicy->Idle.Action) and the user has been inactive past
 *    IdleTimeout, request the power action via NtInitiatePowerAction so the
 *    system enters sleep, hibernate, or shuts down.
 *
 *    Full idle-counter tracking (comparing elapsed user-inactivity time
 *    against IdleTimeout) requires access to Win32k's user-input timestamp,
 *    which is not yet wired back to the kernel Power Manager. The call to
 *    NtInitiatePowerAction below is therefore gated by whether a non-zero
 *    IdleTimeout is set; once the user-inactivity plumbing is in place the
 *    comparison should be made here before initiating the action.
 */
VOID
NTAPI
PopPowerPolicySystemIdle(VOID)
{
    WIN32_POWEREVENT_PARAMETERS Params;
    ULONG VideoTimeout;
    POWER_ACTION IdleAction;

    PAGED_CODE();

    /*
     * Notify Win32k that the active power policy (and therefore the video
     * and system idle timeouts) may have changed. Win32k will re-arm its
     * internal countdown timers with the new timeout values from the policy.
     *
     * PsW32PowerPolicyChanged causes Win32k to re-read the active policy via
     * NtPowerInformation(SystemPowerPolicyCurrent) and update its display-off
     * and system-idle countdowns accordingly.
     */
    Params.EventNumber = PsW32PowerPolicyChanged;
    Params.Code = 0;
    PopDispatchPowerEvent(&Params);

    /*
     * If video dimming or blanking is supported and the policy's VideoTimeout
     * is non-zero, signal Win32k to prepare for a GDI power-off sequence.
     * PsW32GdiOffRequest tells Win32k to begin the "display about to turn off"
     * transition (fade-out, lock screen if WinLogonFlags say so, etc.).
     * PsW32MonitorOff follows to actually cut the monitor backlight.
     */
    VideoTimeout = PopDefaultPowerPolicy->VideoTimeout;
    if (PopCapabilities.VideoDimPresent && VideoTimeout > 0)
    {
        DPRINT("PopPowerPolicySystemIdle: VideoTimeout %lu s reached, requesting display off\n",
               VideoTimeout);

        Params.EventNumber = PsW32GdiOffRequest;
        Params.Code = 0;
        PopDispatchPowerEvent(&Params);

        Params.EventNumber = PsW32MonitorOff;
        Params.Code = 0;
        PopDispatchPowerEvent(&Params);
    }

    /*
     * If the policy specifies a non-None idle power action and the idle
     * timeout is non-zero, initiate the configured power action
     * (e.g. PowerActionSleep -> S3, PowerActionHibernate -> S4).
     *
     * The POWER_ACTION_QUERY_ALLOWED and POWER_ACTION_UI_ALLOWED flags ensure
     * applications and services are given a chance to veto the transition
     * (e.g. a media player can reject the sleep request).
     *
     * TODO: Gate this behind an actual user-inactivity elapsed-time check once
     * Win32k reports idle duration back to the kernel Power Manager.
     */
    IdleAction = PopDefaultPowerPolicy->Idle.Action;
    if (IdleAction != PowerActionNone && PopDefaultPowerPolicy->IdleTimeout > 0)
    {
        DPRINT("PopPowerPolicySystemIdle: IdleTimeout %lu s reached, initiating action %d\n",
               PopDefaultPowerPolicy->IdleTimeout, IdleAction);

        NtInitiatePowerAction(IdleAction,
                              PopDefaultPowerPolicy->MinSleep,
                              PopDefaultPowerPolicy->Idle.Flags |
                              POWER_ACTION_QUERY_ALLOWED |
                              POWER_ACTION_UI_ALLOWED,
                              FALSE);
    }
}

/**
 * @brief
 * A power policy function that's called whenever the time
 * has changed.
 */
VOID
NTAPI
PopPowerPolicyTimeChange(VOID)
{
    WIN32_POWEREVENT_PARAMETERS Params;

    /* Invoke the Win32k power callout to dispatch the time change event */
    Params.EventNumber = PsW32SystemTime;
    Params.Code = 0;
    PopDispatchPowerEvent(&Params);
}

/**
 * @brief
 * Initializes a power policy structures.
 *
 * @param[out] PowerPolicy
 * A pointer to a power policy structure that is to
 * be filled with default policy data.
 */
VOID
NTAPI
PopInitializePowerPolicy(
    _Out_ PSYSTEM_POWER_POLICY PowerPolicy)
{
    ULONG PolicyIndex;

    PAGED_CODE();

    /*
     * Zero out the structure and set the policy revision. Up to this day
     * the Revision 1 is universal across all editions of Windows. This is
     * unlikely to change, unless a huge revamp occurs in the power manager.
     */
    RtlZeroMemory(PowerPolicy, sizeof(SYSTEM_POWER_POLICY));
    PowerPolicy->Revision = POP_SYSTEM_POWER_POLICY_REVISION_V1;

    /* Let Winlogon behave normally for this policy (no logon lock on sleep) */
    PowerPolicy->WinLogonFlags = 0;

    /* Set the default power actions and states for this policy */
    PowerPolicy->LidOpenWake = PowerSystemWorking;
    PowerPolicy->LidClose.Action = PowerActionNone;
    PowerPolicy->ReducedLatencySleep = PowerSystemSleeping1;
    PowerPolicy->MinSleep = PowerSystemSleeping1;
    PowerPolicy->MaxSleep = PowerSystemSleeping3;
    PopSetButtonPowerAction(&PowerPolicy->PowerButton, PowerActionShutdownOff);
    PopSetButtonPowerAction(&PowerPolicy->SleepButton, PowerActionSleep);

    /* Setup the default minimum sleep state for all the discharge policies */
    for (PolicyIndex = 0; PolicyIndex < NUM_DISCHARGE_POLICIES; PolicyIndex++)
    {
        PowerPolicy->DischargePolicy[PolicyIndex].MinSystemState = PowerSystemSleeping1;
    }

    /* Set the default fan throttle constants */
    PowerPolicy->FanThrottleTolerance = 100;
    PowerPolicy->ForcedThrottle = 100;
    PowerPolicy->OverThrottled.Action = PowerActionNone;

    /*
     * Let the system be notified of 25 resolutions of a system
     * power state change event.
     */
    PowerPolicy->BroadcastCapacityResolution = 25;
}

/**
 * @brief
 * Defaults the Power Manager policies by reading the policy data
 * from the registry. Any data not present in the registry is
 * defaulted to initial values established by PopInitializePowerPolicy.
 */
VOID
NTAPI
PopDefaultPolicies(VOID)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    ULONG Length;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(SYSTEM_POWER_POLICY)];
    PKEY_VALUE_PARTIAL_INFORMATION Info = (PVOID)Buffer;

    PAGED_CODE();

    /*
     * Read the shutdown policy. This policy will tell us whether the system must
     * be powered off after shutdown or not.
     */
    PopReadShutdownPolicy();

    /*
     * Initialize both the AC and DC policies to sensible default values
     * before attempting to load them from the registry. This ensures the
     * system is always in a consistent state even when the registry has
     * incomplete or missing policy data.
     */
    PopInitializePowerPolicy(&PopAcPowerPolicy);
    PopInitializePowerPolicy(&PopDcPowerPolicy);

    /* Setup object attributes for the power registry key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PopPowerRegPath,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Attempt to open the power registry key */
    Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /*
         * The power registry key does not exist or is inaccessible.
         * This is expected on a first boot or a minimal installation.
         * The default policies initialised above will be used instead.
         */
        DPRINT("PopDefaultPolicies: Power registry key not found (Status 0x%lx), using defaults\n", Status);
        goto ApplyDefaults;
    }

    /*
     * Attempt to read the AC (mains power) policy from the registry.
     * The value is stored as a raw binary blob of SYSTEM_POWER_POLICY size.
     */
    Status = ZwQueryValueKey(KeyHandle,
                             &RegAcPolicy,
                             KeyValuePartialInformation,
                             Info,
                             sizeof(Buffer),
                             &Length);
    if (NT_SUCCESS(Status) &&
        Info->Type == REG_BINARY &&
        Info->DataLength == sizeof(SYSTEM_POWER_POLICY))
    {
        /* Validate the revision field before accepting the stored AC policy */
        PSYSTEM_POWER_POLICY StoredAcPolicy = (PSYSTEM_POWER_POLICY)Info->Data;
        if (StoredAcPolicy->Revision == POP_SYSTEM_POWER_POLICY_REVISION_V1)
        {
            RtlCopyMemory(&PopAcPowerPolicy, StoredAcPolicy, sizeof(SYSTEM_POWER_POLICY));
            DPRINT("PopDefaultPolicies: AC power policy loaded from registry\n");
        }
        else
        {
            DPRINT1("PopDefaultPolicies: AC policy revision mismatch (%lu), using default\n",
                    StoredAcPolicy->Revision);
        }
    }
    else
    {
        DPRINT("PopDefaultPolicies: AC policy not found or invalid (Status 0x%lx), using default\n", Status);
    }

    /*
     * Attempt to read the DC (battery power) policy from the registry.
     */
    Status = ZwQueryValueKey(KeyHandle,
                             &RegDcPolicy,
                             KeyValuePartialInformation,
                             Info,
                             sizeof(Buffer),
                             &Length);
    if (NT_SUCCESS(Status) &&
        Info->Type == REG_BINARY &&
        Info->DataLength == sizeof(SYSTEM_POWER_POLICY))
    {
        PSYSTEM_POWER_POLICY StoredDcPolicy = (PSYSTEM_POWER_POLICY)Info->Data;
        if (StoredDcPolicy->Revision == POP_SYSTEM_POWER_POLICY_REVISION_V1)
        {
            RtlCopyMemory(&PopDcPowerPolicy, StoredDcPolicy, sizeof(SYSTEM_POWER_POLICY));
            DPRINT("PopDefaultPolicies: DC power policy loaded from registry\n");
        }
        else
        {
            DPRINT1("PopDefaultPolicies: DC policy revision mismatch (%lu), using default\n",
                    StoredDcPolicy->Revision);
        }
    }
    else
    {
        DPRINT("PopDefaultPolicies: DC policy not found or invalid (Status 0x%lx), using default\n", Status);
    }

    ZwClose(KeyHandle);

ApplyDefaults:
    /*
     * The default power policy is the AC policy. If the system is running on
     * battery power (DC), PopIsSystemDrainingAcSource will switch this over
     * to the DC policy once composite battery information is available.
     */
    PopDefaultPowerPolicy = &PopAcPowerPolicy;
    DPRINT("PopDefaultPolicies: Default power policy set to AC\n");
}

/**
 * @brief
 * Propagates processor-relevant fields from the currently active
 * @c SYSTEM_POWER_POLICY into the kernel PPM policy globals and
 * notifies any registered power-setting callbacks.
 *
 * This function must be called whenever the active power policy changes:
 *   - After @c PopDefaultPolicies() loads the initial policy from the registry
 *     during Phase 1 Power Manager initialisation.
 *   - When the AC/DC power source changes (so the alternate policy's throttle
 *     floor takes effect immediately).
 *   - When user-mode calls @c NtPowerInformation to install a new policy.
 *
 * @remarks
 * Callers must ensure @c PopDefaultPowerPolicy is non-NULL before calling.
 * The function is intentionally lightweight: it copies one byte from the
 * active policy and triggers a work-item chain only if callbacks are
 * already registered.  During early boot the notification is effectively
 * a no-op (no callbacks registered yet), but the global update still
 * ensures the PPM engine starts with the correct floor.
 *
 * Must be called at PASSIVE_LEVEL.
 */
VOID
NTAPI
PopSyncPpmPolicyFromCurrentPolicy(VOID)
{
    PAGED_CODE();

    if (PopDefaultPowerPolicy == NULL)
    {
        DPRINT("PopSyncPpmPolicyFromCurrentPolicy: no active policy yet\n");
        return;
    }

    /*
     * SYSTEM_POWER_POLICY.MinThrottle is the minimum throttle percentage
     * (0 – 100 %) that the processor may be clocked down to under this
     * scheme.  It is the direct analogue of GUID_PROCESSOR_THROTTLE_MINIMUM
     * inside a SYSTEM_POWER_POLICY structure.
     *
     * Update the PPM global so that the P-state engine observes the new
     * floor starting from the very next 20 ms DPC period.
     */
    PpmPolicyMinThrottle = PopDefaultPowerPolicy->MinThrottle;

    DPRINT("PopSyncPpmPolicyFromCurrentPolicy: MinThrottle=%u%%\n",
           (ULONG)PpmPolicyMinThrottle);

    PpmResetIdleAccountingAllProcessors();

    /*
     * Notify any registered GUID_PROCESSOR_THROTTLE_MINIMUM callbacks so
     * that user-mode power management components and processor drivers can
     * adjust their own state.  If no callbacks are registered yet (early
     * boot path) this is a harmless no-op.
     */
    PopNotifyPowerSettingChange(&GUID_PROCESSOR_THROTTLE_MINIMUM);

    /*
     * Also notify the GUID_SYSTEM_COOLING_POLICY callbacks because the
     * SYSTEM_POWER_POLICY that was just applied may imply a different
     * thermal cooling mode (e.g. AC = active cooling, DC = passive).
     *
     * Note: PopCoolingSystemMode is updated by the thermal zone manager
     * (thrmzn.c) independently; we trigger the notification here so that
     * registered drivers see the change in a timely fashion.
     */
    PopNotifyPowerSettingChange(&GUID_SYSTEM_COOLING_POLICY);
}

/**
 * @brief
 * Registers a power policy worker. The Power Policy Manager
 * calls this worker whenever there are changes to be made
 * in a policy due to certain circumstances or conditions
 * such as the system being idle.
 *
 * @param[in] WorkerType
 * The type of policy worker of which a worker function
 * is to be assigned.
 *
 * @param[in] WorkerFunction
 * A pointer to a worker function that represents the worker
 * routine itself for the policy worker.
 */
VOID
NTAPI
PopRegisterPowerPolicyWorker(
    _In_ POP_POWER_POLICY_WORKER_TYPES WorkerType,
    _In_ PPOP_POLICY_WORKER_FUNC WorkerFunction)
{
    PAGED_CODE();

    /* Do not let the caller on tricking us with an unknown policy type */
    ASSERT((WorkerType >= PolicyWorkerNotification) && (WorkerType <= PolicyWorkerMax));

    /* Register a policy worker of this type */
    PopPolicyWorker[WorkerType].Pending = FALSE;
    PopPolicyWorker[WorkerType].Thread = NULL;
    PopPolicyWorker[WorkerType].WorkerFunction = WorkerFunction;
}

/**
 * @brief
 * Requests a policy worker thread to handle power policy events.
 * Only one worker per policy can be requested.
 *
 * @param[in] WorkerType
 * The type of policy worker to be requested.
 */
VOID
NTAPI
PopRequestPolicyWorker(
    _In_ POP_POWER_POLICY_WORKER_TYPES WorkerType)
{
    BOOLEAN PendingWorker;
    KIRQL Irql;

    /*
     * Check if this kind of worker type has not been requested before.
     * By rule there can be only one worker enqueued for each type.
     * It does not matter if somebody wants to enqueue a worker that is
     * already pending, because once a policy worker fires up it will
     * address the latest changes that will take effect on the system.
     */
    PopAcquirePowerPolicyWorkerLock(&Irql);
    PendingWorker = PopPolicyWorker[WorkerType].Pending;
    if (!PendingWorker)
    {
        /* This type of worker was never requested, set the pending mark */
        PopPolicyWorker[WorkerType].Pending = TRUE;

        /*
         * For debugging purposes attach the current calling thread to
         * this policy worker so that we know which thread requested it before.
         */
        PopPolicyWorker[WorkerType].Thread = KeGetCurrentThread();
    }

    PopReleasePowerPolicyWorkerLock(Irql);
}

/**
 * @brief
 * Checks for any pending policy requests to be processed by
 * the power policy worker thread. The policy worker thread
 * is enqueued only when it's not already running. This is to
 * ensure that policy changes, notifications and alike are
 * properly synchronized.
 */
VOID
NTAPI
PopCheckForPendingWorkers(VOID)
{
    ULONG WorkerIndex;
    KIRQL Irql;
    BOOLEAN PendingFound = FALSE;

    /*
     * The current calling thread owns the policy lock. This means the
     * thread is currently doing changes at certain power policies.
     * We will dispatch any outstanding workers as soon as the calling
     * thread leaves the policy manager.
     */
    if (PopPowerPolicyOwnerLockThread == KeGetCurrentThread())
    {
        return;
    }

    /* Do not allow any further worker requests, check for any pending work */
    PopAcquirePowerPolicyWorkerLock(&Irql);
    for (WorkerIndex = PolicyWorkerNotification;
         WorkerIndex < PolicyWorkerMax;
         WorkerIndex++)
    {
        /* Is this worker pending? */
        if (PopPolicyWorker[WorkerIndex].Pending)
        {
            /* It is, stop locking */
            PendingFound = TRUE;
            break;
        }
    }

    /* If no outstanding work was found then just quit */
    if (!PendingFound)
    {
        PopReleasePowerPolicyWorkerLock(Irql);
        return;
    }

    /*
     * At least one policy worker has been requested and currently
     * pending for dispatch. Check that the policy manager worker
     * thread is not already running. Enqueue it if need be.
     */
    if (!PopPendingPolicyWorker)
    {
        PopPendingPolicyWorker = TRUE;
        ExQueueWorkItem(&PopPowerPolicyWorkItem, DelayedWorkQueue);
    }

    /* Allow worker requests now */
    PopReleasePowerPolicyWorkerLock(Irql);
}

/**
 * @brief
 * The main core worker thread that executes policy routines
 * depending on the policy requests made by the Power Manager.
 *
 * @param[in] WorkerType
 * A pointer to a context parameter. This is unused.
 */
_Use_decl_annotations_
VOID
NTAPI
PopPolicyManagerWorker(
    _In_ PVOID Parameter)
{
    ULONG WorkerIndex;
    KIRQL Irql;

    /*
     * Iterate over the registered policy workers and look for any of these
     * that are currently pending for dispatch.
     */
    PopAcquirePowerPolicyWorkerLock(&Irql);
    for (WorkerIndex = PolicyWorkerNotification;
         WorkerIndex < PolicyWorkerMax;
         WorkerIndex++)
    {
        /* Process this policy worker only if it has been requested */
        if (PopPolicyWorker[WorkerIndex].Pending)
        {
            /* Clear the pending mark, we will serve this policy worker now */
            PopPolicyWorker[WorkerIndex].Pending = FALSE;
            PopPolicyWorker[WorkerIndex].Thread = NULL;
            PopReleasePowerPolicyWorkerLock(Irql);

            /* Dispatch it */
            PopPolicyWorker[WorkerIndex].WorkerFunction();

            /* Look for the next policy worker */
            PopAcquirePowerPolicyWorkerLock(&Irql);
        }
    }

    /* The policy manager worker thread has finished its job */
    PopPendingPolicyWorker = FALSE;
    PopReleasePowerPolicyWorkerLock(Irql);
}

/**
 * @brief
 * Callback routine executed by the I/O manager whenever a
 * device policy is being added or removed from the system.
 * The Power Manager will invoke the Power Policy Manager (PPM)
 * to do actions according to the notification sent by the I/O manager.
 *
 * @param[in] NotificationStructure
 * A pointer to notification structure, passed by the I/O manager.
 * The Power Manager uses this notification packet to understand if
 * this is an upcoming policy device or it's being removed. The
 * Policy Manager will register this upcoming device.
 *
 * @param[in] Context
 * A pointer to arbitrary data, pointing to a power policy device
 * type. Examples of policy device types can be a system button,
 * thermal zone, composite battery, et al.
 *
 * @return
 * Returns STATUS_SUCCESS if the device policy notifaction has been handled
 * successfully. STATUS_INVALID_PARAMETER is returned if no notification
 * packet was provided or its contents are invalid. STATUS_REVISION_MISMATCH
 * is returned if the notification structure has a different version than what
 * we support. A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
PopDevicePolicyCallback(
    _In_ PVOID NotificationStructure,
    _In_ PVOID Context)
{
    NTSTATUS Status;
    BOOLEAN Arrival;
    POWER_POLICY_DEVICE_TYPE PolicyDeviceType;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification;

    PAGED_CODE();

    /* Passing a NULL policy notification is not what we expect here */
    if (!NotificationStructure)
    {
        DPRINT1("Got NULL policy notification context\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Begin validating the notification structure */
    Notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;
    PolicyDeviceType = (POWER_POLICY_DEVICE_TYPE)(ULONG_PTR)Context;
    if (Notification->Version != 1)
    {
        DPRINT1("The policy notification has an invalid version (must be 1, got %u)\n", Notification->Version);
        return STATUS_REVISION_MISMATCH;
    }

    if (Notification->Size != sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION))
    {
        DPRINT1("The policy notification has a malformed size structure (must be %u, got %u)\n",
                sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION), Notification->Size);
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Now check whether we have an upcoming policy device that we must connect
     * to or detach the said policy device with the power manager. Any events
     * other than arrival or removal notifications are bogus.
     */
    if (RtlCompareMemory(&Notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL, sizeof(GUID)) == sizeof(GUID))
    {
        Arrival = TRUE;
        PopAcquirePowerPolicyLock();
        Status = PopAddPolicyDevice(Notification, PolicyDeviceType);
        PopReleasePowerPolicyLock();
    }
    else if (RtlCompareMemory(&Notification->Event, &GUID_DEVICE_INTERFACE_REMOVAL, sizeof(GUID)) == sizeof(GUID))
    {
        Arrival = FALSE;
        PopAcquirePowerPolicyLock();
        Status = PopRemovePolicyDevice(Notification, PolicyDeviceType);
        PopReleasePowerPolicyLock();
    }
    else
    {
        DPRINT1("Got a bogus policy notification event, must be either be ARRIVAL or REMOVAL\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Bail out if the policy device processing has failed */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("The %s of policy device %wZ has failed (Status 0x%lx)\n",
                Arrival ? "arrival" : "removal", Notification->SymbolicLinkName, Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

/* EOF */

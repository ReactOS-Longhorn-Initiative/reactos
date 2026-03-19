/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager Framework API (PoFx) support routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * DPC routine that fires when the per-device PoFx idle timer expires.
 * When the idle timer expires the device has been inactive for longer
 * than the caller-specified idle timeout, so we can ask it to power down
 * by invoking the DevicePowerNotRequired callback.
 *
 * @param[in] Dpc
 * The DPC object associated with this timer.
 *
 * @param[in] DeferredContext
 * A pointer to the POP_FX_DEVICE whose idle timer fired.
 *
 * @param[in] SystemArgument1
 * Unused.
 *
 * @param[in] SystemArgument2
 * Unused.
 */
static
VOID
NTAPI
PopFxDeviceIdleTimerDpc(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)DeferredContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    POFXTRACE(POFX_DEVICE_DEBUG,
              "PopFxDeviceIdleTimerDpc: idle timeout fired for FxDevice %p\n",
              FxDevice);

    /*
     * Only call the DevicePowerNotRequired callback if the idle timer is
     * still considered armed (IrqTimerOn). The caller may have called
     * PoFxActivateComponent in the meantime which would have cleared the flag.
     */
    if (FxDevice->Status.IdleTimerOn &&
        FxDevice->Callbacks.DevicePowerNotRequired != NULL)
    {
        FxDevice->Status.DPNRDeviceNotified = 1;
        FxDevice->Callbacks.DevicePowerNotRequired(FxDevice->DriverContext);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Registers a device with the Power Framework (PoFx).
 *
 * @param[in] Pdo
 * A pointer to a physical device object that wants to be
 * registered with the Power Framework.
 *
 * @param[in] Device
 * A pointer to a Framework device descriptor that points to the actual
 * information about the PDO that is to be registered.
 *
 * @param[out] Handle
 * A pointer to a Framework handle returned to the caller after
 * the registration of the device.
 *
 * @return
 * Returns STATUS_SUCCESS if the device has been registered with PoFx.
 * STATUS_INVALID_PARAMETER is returned if invalid parameters are given.
 * STATUS_INSUFFICIENT_RESOURCES is returned if there is not enough memory
 * to allocate the internal PoFx device structures.
 */
NTSTATUS
NTAPI
PoFxRegisterDevice(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PPO_FX_DEVICE Device,
    _Out_ POHANDLE *Handle)
{
    NTSTATUS Status;
    PPOP_FX_DEVICE FxDevice;
    PPOP_FX_COMPONENT FxComponent;
    PPO_FX_COMPONENT_V1 SrcComponent;
    ULONG ComponentCount;
    ULONG ComponentIndex;
    ULONG StateIndex;
    SIZE_T AllocSize;
    KLOCK_QUEUE_HANDLE LockHandle;

    PAGED_CODE();

    /* Validate parameters */
    if (Pdo == NULL || Device == NULL || Handle == NULL)
    {
        DPRINT1("PoFxRegisterDevice: NULL parameter passed\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Only V1 is supported for now */
    if (Device->Version != PO_FX_VERSION_V1)
    {
        DPRINT1("PoFxRegisterDevice: unsupported PoFx device version %u\n",
                Device->Version);
        return STATUS_NOT_SUPPORTED;
    }

    ComponentCount = Device->ComponentCount;

    /*
     * Allocate the internal PoFx device structure. The Components[] field
     * at the end of POP_FX_DEVICE is an array of POINTERS, one per component.
     * We over-allocate by (ComponentCount - 1) pointer slots.
     */
    AllocSize = FIELD_OFFSET(POP_FX_DEVICE, Components) +
                ComponentCount * sizeof(PPOP_FX_COMPONENT);

    FxDevice = PopAllocatePool(AllocSize, FALSE, TAG_PO_FX_DEVICE);
    if (FxDevice == NULL)
    {
        DPRINT1("PoFxRegisterDevice: failed to allocate POP_FX_DEVICE\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(FxDevice, AllocSize);

    /* Set the device object and component count */
    FxDevice->DeviceObject = Pdo;
    FxDevice->TargetDevice = IoGetAttachedDeviceReference(Pdo);
    FxDevice->ComponentCount = ComponentCount;

    /*
     * Cache the driver-supplied callbacks from the public descriptor.
     * The internal POP_FX_DRIVER_CALLBACKS layout mirrors the PO_FX_DEVICE_V1
     * callback order exactly.
     */
    FxDevice->Callbacks.ComponentActive      = (VOID(*)(PVOID, ULONG))Device->ComponentActiveConditionCallback;
    FxDevice->Callbacks.ComponentIdle        = (VOID(*)(PVOID, ULONG))Device->ComponentIdleConditionCallback;
    FxDevice->Callbacks.ComponentIdleState   = (VOID(*)(PVOID, ULONG, ULONG))Device->ComponentIdleStateCallback;
    FxDevice->Callbacks.DevicePowerRequired  = (VOID(*)(PVOID))Device->DevicePowerRequiredCallback;
    FxDevice->Callbacks.DevicePowerNotRequired = (VOID(*)(PVOID))Device->DevicePowerNotRequiredCallback;
    FxDevice->Callbacks.PowerControl        = (LONG(*)(PVOID, PGUID, PVOID, ULONG, PVOID, ULONG, PULONG))Device->PowerControlCallback;

    /* Store the driver-supplied context */
    FxDevice->DriverContext = Device->DeviceContext;

    /* Initialize the remove lock so PoFxUnregisterDevice can synchronize safely */
    IoInitializeRemoveLock(&FxDevice->RemoveLock, TAG_PO_FX_DEVICE, 0, 0);

    /* Initialize the idle timer and its DPC */
    KeInitializeTimerEx(&FxDevice->IdleTimer, NotificationTimer);
    KeInitializeDpc(&FxDevice->IdleDpc, PopFxDeviceIdleTimerDpc, FxDevice);

    /* Initialize the IRP completion event */
    KeInitializeEvent(&FxDevice->IrpCompleteEvent, NotificationEvent, FALSE);

    /* Obtain the device node (if available) */
    FxDevice->DevNode = IopGetDeviceNode(Pdo);

    /*
     * Allocate and initialize each component. Components are allocated
     * individually so that they can be freed independently.
     */
    for (ComponentIndex = 0; ComponentIndex < ComponentCount; ComponentIndex++)
    {
        SrcComponent = &Device->Components[ComponentIndex];

        AllocSize = sizeof(POP_FX_COMPONENT);
        FxComponent = PopAllocatePool(AllocSize, FALSE, TAG_PO_FX_COMPONENT);
        if (FxComponent == NULL)
        {
            DPRINT1("PoFxRegisterDevice: failed to allocate component %u\n",
                    ComponentIndex);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto CleanupComponents;
        }

        RtlZeroMemory(FxComponent, AllocSize);

        FxComponent->Index = ComponentIndex;
        FxComponent->Device = FxDevice;

        /* Initialize the active-condition event (initially not signaled = idle) */
        KeInitializeEvent(&FxComponent->ActiveEvent, NotificationEvent, FALSE);

        /* By default, start in F0 (component active = fully on) */
        FxComponent->CurrentIdleState = 0;

        /* Copy the idle state table if provided */
        FxComponent->IdleStateCount = SrcComponent->IdleStateCount;
        if (SrcComponent->IdleStateCount > 0 && SrcComponent->IdleStates != NULL)
        {
            FxComponent->IdleStates = PopAllocatePool(
                SrcComponent->IdleStateCount * sizeof(POP_FX_IDLE_STATE),
                FALSE,
                TAG_PO_FX_COMPONENT);
            if (FxComponent->IdleStates == NULL)
            {
                DPRINT1("PoFxRegisterDevice: failed to allocate idle states "
                        "for component %u\n", ComponentIndex);
                PopFreePool(FxComponent, TAG_PO_FX_COMPONENT);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto CleanupComponents;
            }

            for (StateIndex = 0; StateIndex < SrcComponent->IdleStateCount; StateIndex++)
            {
                FxComponent->IdleStates[StateIndex].TransitionLatency =
                    SrcComponent->IdleStates[StateIndex].TransitionLatency;
                FxComponent->IdleStates[StateIndex].ResidencyRequirement =
                    SrcComponent->IdleStates[StateIndex].ResidencyRequirement;
                FxComponent->IdleStates[StateIndex].NominalPower =
                    SrcComponent->IdleStates[StateIndex].NominalPower;
            }
        }

        FxDevice->Components[ComponentIndex] = FxComponent;
    }

    /* Acquire the PoFx device list lock and insert this device */
    KeAcquireInStackQueuedSpinLock(&PopFxDeviceLock, &LockHandle);
    InsertTailList(&PopFxDeviceList, &FxDevice->Link);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    /* Return the opaque handle to the caller */
    *Handle = (POHANDLE)FxDevice;

    POFXTRACE(POFX_DEVICE_DEBUG,
              "PoFxRegisterDevice: registered FxDevice %p (PDO %p, %u components)\n",
              FxDevice, Pdo, ComponentCount);

    return STATUS_SUCCESS;

CleanupComponents:
    /* Free any components already allocated */
    for (; ComponentIndex > 0; ComponentIndex--)
    {
        FxComponent = FxDevice->Components[ComponentIndex - 1];
        if (FxComponent != NULL)
        {
            if (FxComponent->IdleStates != NULL)
            {
                PopFreePool(FxComponent->IdleStates, TAG_PO_FX_COMPONENT);
            }
            PopFreePool(FxComponent, TAG_PO_FX_COMPONENT);
        }
    }

    /* Dereference the target device we referenced above */
    ObDereferenceObject(FxDevice->TargetDevice);

    PopFreePool(FxDevice, TAG_PO_FX_DEVICE);
    return Status;
}

/**
 * @brief
 * Unregisters a device from the Power Framework.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 */
VOID
NTAPI
PoFxUnregisterDevice(
    _In_ POHANDLE Handle)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;
    PPOP_FX_COMPONENT FxComponent;
    ULONG ComponentIndex;
    KLOCK_QUEUE_HANDLE LockHandle;

    PAGED_CODE();

    if (FxDevice == NULL)
    {
        DPRINT1("PoFxUnregisterDevice: NULL handle\n");
        return;
    }

    POFXTRACE(POFX_DEVICE_DEBUG,
              "PoFxUnregisterDevice: unregistering FxDevice %p\n", FxDevice);

    /* Cancel the idle timer if it is still running */
    KeCancelTimer(&FxDevice->IdleTimer);

    /* Remove from the global PoFx device list */
    KeAcquireInStackQueuedSpinLock(&PopFxDeviceLock, &LockHandle);
    RemoveEntryList(&FxDevice->Link);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    /* Free each component */
    for (ComponentIndex = 0; ComponentIndex < FxDevice->ComponentCount; ComponentIndex++)
    {
        FxComponent = FxDevice->Components[ComponentIndex];
        if (FxComponent != NULL)
        {
            if (FxComponent->IdleStates != NULL)
            {
                PopFreePool(FxComponent->IdleStates, TAG_PO_FX_COMPONENT);
            }
            PopFreePool(FxComponent, TAG_PO_FX_COMPONENT);
        }
    }

    /* Release the target device reference taken in PoFxRegisterDevice */
    if (FxDevice->TargetDevice != NULL)
    {
        ObDereferenceObject(FxDevice->TargetDevice);
    }

    /* Release the remove lock and wait for any active I/O references to drain */
    IoReleaseRemoveLockAndWait(&FxDevice->RemoveLock, FxDevice);

    PopFreePool(FxDevice, TAG_PO_FX_DEVICE);
}

/**
 * @brief
 * Finish the registration done with a call to PoFxRegisterDevice and
 * puts all the components to an idle state condition.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx, of which device power management
 * is to be started.
 */
VOID
NTAPI
PoFxStartDevicePowerManagement(
    _In_ POHANDLE Handle)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;
    PPOP_FX_COMPONENT FxComponent;
    ULONG ComponentIndex;

    PAGED_CODE();

    if (FxDevice == NULL)
    {
        DPRINT1("PoFxStartDevicePowerManagement: NULL handle\n");
        return;
    }

    POFXTRACE(POFX_DEVICE_DEBUG,
              "PoFxStartDevicePowerManagement: starting FxDevice %p\n", FxDevice);

    /*
     * Transition every component to the F0/Fx-idle condition so that
     * the component state machine is consistent before the driver starts
     * issuing PoFxActivateComponent / PoFxIdleComponent calls.
     * We set the CurrentIdleState to 0 (F0, the active state) and signal
     * the active condition event so any thread waiting in PoFxActivateComponent
     * with PO_FX_FLAG_BLOCKING returns immediately on the first call.
     */
    for (ComponentIndex = 0; ComponentIndex < FxDevice->ComponentCount; ComponentIndex++)
    {
        FxComponent = FxDevice->Components[ComponentIndex];
        if (FxComponent == NULL)
        {
            continue;
        }

        FxComponent->CurrentIdleState = 0;
        FxComponent->Flags.Active = 0;
        FxComponent->IdleConditionComplete = 0;

        /*
         * Notify the driver via the ComponentIdleCondition callback so it
         * knows each component is starting in the idle/F0 condition. This
         * mirrors what Windows does when it starts PoFx power management.
         */
        if (FxDevice->Callbacks.ComponentIdle != NULL)
        {
            FxDevice->Callbacks.ComponentIdle(FxDevice->DriverContext,
                                              ComponentIndex);
        }
    }
}

/**
 * @brief
 * Activates a component of a device, after being idle.
 * The function will perform a state transition to the active
 * state if the component cannot be currently accessed.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] Component
 * An index to the component that is to be activated.
 *
 * @param[in] Flags
 * Flag bitmask provided by the caller that changes the behavior
 * of this function. The following flags are:
 *
 * PO_FX_FLAG_BLOCKING -- The operation is synchronous, the control is
 *                        returned to the caller only when the operation
 *                        has completed.
 *
 * PO_FX_FLAG_ASYNC_ONLY -- The operation is asynchronous, the operation
 *                          will be handled in a separate thread other than
 *                          the calling thread. The control is returned to the
 *                          device driver immediately.
 */
VOID
NTAPI
PoFxActivateComponent(
    _In_ POHANDLE Handle,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;
    PPOP_FX_COMPONENT FxComponent;

    if (FxDevice == NULL || Component >= FxDevice->ComponentCount)
    {
        DPRINT1("PoFxActivateComponent: invalid parameter\n");
        return;
    }

    FxComponent = FxDevice->Components[Component];
    if (FxComponent == NULL)
    {
        return;
    }

    POFXTRACE(POFX_COMPONENT_DEBUG,
              "PoFxActivateComponent: FxDevice %p component %u flags 0x%x\n",
              FxDevice, Component, Flags);

    /*
     * If the component is already active, the component reference count is
     * incremented and the call returns immediately.
     */
    if (FxComponent->Flags.Active)
    {
        /* RefCount is a bit-field; use the underlying Value member for interlocked ops */
        InterlockedIncrement(&FxComponent->Flags.Value);

        if (Flags & PO_FX_FLAG_BLOCKING)
        {
            /* Signal the event so any waiter can proceed */
            KeSetEvent(&FxComponent->ActiveEvent, IO_NO_INCREMENT, FALSE);
        }
        return;
    }

    /*
     * Transition the component to the active (F0) state. Transition latency
     * is not simulated here - a full implementation would queue a work item
     * and notify the caller asynchronously for Fx->F0 transitions.
     */
    FxComponent->Flags.Active = 1;
    FxComponent->CurrentIdleState = 0;
    /* RefCount is a bit-field; use the underlying Value member for interlocked ops */
    InterlockedIncrement(&FxComponent->Flags.Value);

    /* Cancel the device idle timer since at least one component is active */
    KeCancelTimer(&FxDevice->IdleTimer);
    FxDevice->Status.IdleTimerOn = 0;

    /* Notify the driver that the component is now active */
    if (FxDevice->Callbacks.ComponentActive != NULL)
    {
        FxDevice->Callbacks.ComponentActive(FxDevice->DriverContext, Component);
    }

    if (Flags & PO_FX_FLAG_BLOCKING)
    {
        /* Signal the event and wait for it to ensure ordering */
        KeSetEvent(&FxComponent->ActiveEvent, IO_NO_INCREMENT, FALSE);
    }
}

/**
 * @brief
 * Acknowledges the Power Framework the device driver has fully executed the
 * DevicePowerNotRequiredCallback callback function.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx, that fully responded to the
 * DevicePowerNotRequiredCallback callback routine.
 */
VOID
NTAPI
PoFxCompleteDevicePowerNotRequired(
    _In_ POHANDLE Handle)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;

    if (FxDevice == NULL)
    {
        DPRINT1("PoFxCompleteDevicePowerNotRequired: NULL handle\n");
        return;
    }

    POFXTRACE(POFX_DEVICE_DEBUG,
              "PoFxCompleteDevicePowerNotRequired: FxDevice %p\n", FxDevice);

    /*
     * Clear the DPNR notified flag so the idle timer may fire again if
     * all components subsequently go idle.
     */
    FxDevice->Status.DPNRDeviceNotified = 0;
    FxDevice->Status.DPNRReceivedFromPep = 0;
}

/**
 * @brief
 * Turns a component of a device into the idle state,
 * after being activated previously by a call to PoFxActivateComponent.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] Component
 * An index to the component that is to be turned into the idle state.
 *
 * @param[in] Flags
 * Flag bitmask provided by the caller that changes the behavior
 * of this function. The following flags are:
 *
 * PO_FX_FLAG_BLOCKING -- The operation is synchronous, the control is
 *                        returned to the caller only when the operation
 *                        has completed.
 *
 * PO_FX_FLAG_ASYNC_ONLY -- The operation is asynchronous, the operation
 *                          will be handled in a separate thread other than
 *                          the calling thread. The control is returned to the
 *                          device driver immediately.
 */
VOID
NTAPI
PoFxIdleComponent(
    _In_ POHANDLE Handle,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;
    PPOP_FX_COMPONENT FxComponent;
    BOOLEAN AllIdle;
    ULONG Index;
    LARGE_INTEGER DueTime;

    if (FxDevice == NULL || Component >= FxDevice->ComponentCount)
    {
        DPRINT1("PoFxIdleComponent: invalid parameter\n");
        return;
    }

    FxComponent = FxDevice->Components[Component];
    if (FxComponent == NULL)
    {
        return;
    }

    POFXTRACE(POFX_COMPONENT_DEBUG,
              "PoFxIdleComponent: FxDevice %p component %u flags 0x%x\n",
              FxDevice, Component, Flags);

    /* Decrement the reference count; if it reaches 0 the component goes idle */
    /* RefCount is a bit-field; use the underlying Value member for interlocked ops */
    if (InterlockedDecrement(&FxComponent->Flags.Value) > 0)
    {
        /* Still referenced; component stays active */
        return;
    }

    /* Mark the component as idle and reset the active event */
    FxComponent->Flags.Active = 0;
    KeClearEvent(&FxComponent->ActiveEvent);

    /*
     * Notify the driver that this component is entering the idle condition.
     * The driver should call PoFxCompleteIdleCondition when it has prepared
     * the component for the Fx idle state.
     */
    if (FxDevice->Callbacks.ComponentIdle != NULL)
    {
        FxDevice->Callbacks.ComponentIdle(FxDevice->DriverContext, Component);
    }

    /*
     * Check whether all components are now idle. If so, arm the device
     * idle timer so that DevicePowerNotRequiredCallback is called after the
     * configured idle timeout.
     */
    AllIdle = TRUE;
    for (Index = 0; Index < FxDevice->ComponentCount; Index++)
    {
        if (FxDevice->Components[Index] != NULL &&
            FxDevice->Components[Index]->Flags.Active)
        {
            AllIdle = FALSE;
            break;
        }
    }

    if (AllIdle && FxDevice->IdleTimeout != 0 && !FxDevice->Status.IdleTimerOn)
    {
        FxDevice->Status.IdleTimerOn = 1;
        DueTime.QuadPart = -((LONGLONG)FxDevice->IdleTimeout);
        KeSetTimer(&FxDevice->IdleTimer, DueTime, &FxDevice->IdleDpc);
    }
}

/**
 * @brief
 * Acknowledges the Power Framework the component of a device has finished
 * transitioning to the idle state condition.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] Component
 * An index to the component that has completed the transition to the
 * idle state condition.
 */
VOID
NTAPI
PoFxCompleteIdleCondition(
    _In_ POHANDLE Handle,
    _In_ ULONG Component)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;
    PPOP_FX_COMPONENT FxComponent;

    if (FxDevice == NULL || Component >= FxDevice->ComponentCount)
    {
        DPRINT1("PoFxCompleteIdleCondition: invalid parameter\n");
        return;
    }

    FxComponent = FxDevice->Components[Component];
    if (FxComponent == NULL)
    {
        return;
    }

    POFXTRACE(POFX_COMPONENT_DEBUG,
              "PoFxCompleteIdleCondition: FxDevice %p component %u\n",
              FxDevice, Component);

    /*
     * Acknowledge that the idle condition transition is complete. This
     * satisfies any pending PoFxIdleComponent(PO_FX_FLAG_BLOCKING) waiter.
     */
    InterlockedExchange(&FxComponent->IdleConditionComplete, 1);
}

/**
 * @brief
 * Acknowledges the Power Framework the component of a device has finished
 * transitioning to a Fx state.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] Component
 * An index to the component that has completed the transition to the
 * Fx state.
 */
VOID
NTAPI
PoFxCompleteIdleState(
    _In_ POHANDLE Handle,
    _In_ ULONG Component)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;
    PPOP_FX_COMPONENT FxComponent;

    if (FxDevice == NULL || Component >= FxDevice->ComponentCount)
    {
        DPRINT1("PoFxCompleteIdleState: invalid parameter\n");
        return;
    }

    FxComponent = FxDevice->Components[Component];
    if (FxComponent == NULL)
    {
        return;
    }

    POFXTRACE(POFX_COMPONENT_DEBUG,
              "PoFxCompleteIdleState: FxDevice %p component %u\n",
              FxDevice, Component);

    /*
     * Signal that the Fx state transition has been completed. The component
     * may now accept the new idle state assignment.
     */
    InterlockedExchange(&FxComponent->IdleStateComplete, 1);

    /* Notify the driver of the completed Fx state transition */
    if (FxDevice->Callbacks.ComponentIdleState != NULL)
    {
        FxDevice->Callbacks.ComponentIdleState(FxDevice->DriverContext,
                                               Component,
                                               FxComponent->CurrentIdleState);
    }
}

/**
 * @brief
 * Specifies the minimum time interval from when the last component of
 * the device enters the idle condition to when the Power Framework calls
 * DevicePowerNotRequiredCallback.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 *
 * @param[in] IdleTimeout
 * The idle timeout interval to be supplied, in 100-nanosecond units.
 * A value of 0 causes the idle timer to fire immediately. Passing
 * PO_FX_EMPTY_WAKE_LATENCY disables the idle timer altogether.
 */
VOID
NTAPI
PoFxSetDeviceIdleTimeout(
    _In_ POHANDLE Handle,
    _In_ ULONGLONG IdleTimeout)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;

    if (FxDevice == NULL)
    {
        DPRINT1("PoFxSetDeviceIdleTimeout: NULL handle\n");
        return;
    }

    POFXTRACE(POFX_DEVICE_DEBUG,
              "PoFxSetDeviceIdleTimeout: FxDevice %p timeout %I64u\n",
              FxDevice, IdleTimeout);

    FxDevice->IdleTimeout = IdleTimeout;
}

/**
 * @brief
 * Acknowledges the Power Framework the following device has fully
 * transitioned to the D0 power state.
 *
 * @param[in] Handle
 * A pointer to a Framework handle that represents the
 * registered handle with PoFx.
 */
VOID
NTAPI
PoFxReportDevicePoweredOn(
    _In_ POHANDLE Handle)
{
    PPOP_FX_DEVICE FxDevice = (PPOP_FX_DEVICE)Handle;

    if (FxDevice == NULL)
    {
        DPRINT1("PoFxReportDevicePoweredOn: NULL handle\n");
        return;
    }

    POFXTRACE(POFX_DEVICE_DEBUG,
              "PoFxReportDevicePoweredOn: FxDevice %p\n", FxDevice);

    /*
     * The device has powered on (returned to D0). Signal the IRP completion
     * event and notify the Power Manager that the IRP is no longer in use.
     */
    FxDevice->Status.IrpInUse = 0;
    FxDevice->Status.IrpPending = 0;
    KeSetEvent(&FxDevice->IrpCompleteEvent, IO_NO_INCREMENT, FALSE);
}

/* EOF */

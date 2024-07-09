/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpnotify.c
 * PURPOSE:         Plug & Play notification functions
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Pierre Schweitzer
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

#define HashGuid(Guid) ((((PULONG)Guid)[0] + ((PULONG)Guid)[1]  + ((PULONG)Guid)[2]  + ((PULONG)Guid)[3]) % 13)

/* TYPES *******************************************************************/

typedef struct _PNP_NOTIFY_ENTRY
{
    LIST_ENTRY PnpNotifyList;
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    PVOID Context;
    UNICODE_STRING Guid;
    PFILE_OBJECT FileObject;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE PnpNotificationProc;
} PNP_NOTIFY_ENTRY, *PPNP_NOTIFY_ENTRY;

/* GLOBALS *******************************************************************/

PSETUP_NOTIFY_DATA IopSetupNotifyData;
LIST_ENTRY PnpNotifyListHead;
KGUARDED_MUTEX PnpNotifyListLock;

extern LIST_ENTRY IopProfileNotifyList;
extern LIST_ENTRY IopDeviceClassNotifyList[13];
extern LIST_ENTRY IopDeferredRegistrationList;

extern KGUARDED_MUTEX IopHwProfileNotifyLock;
extern KGUARDED_MUTEX IopDeviceClassNotifyLock;
extern KGUARDED_MUTEX IopTargetDeviceNotifyLock;
extern KGUARDED_MUTEX IopDeferredRegistrationLock;
extern KGUARDED_MUTEX PiNotificationInProgressLock;

extern BOOLEAN PiNotificationInProgress;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
IopReferenceNotify(
    _In_ PPNP_NOTIFY_HEADER NotifyHeader)
{
    DPRINT("IopReferenceNotify: NotifyHeader %p\n", NotifyHeader);
    PAGED_CODE();

    ASSERT(NotifyHeader);
    ASSERT(NotifyHeader->RefCount);

    NotifyHeader->RefCount++;
}

VOID
NTAPI
IopDereferenceNotify(
    _In_ PPNP_NOTIFY_HEADER NotifyHeader)
{
    DPRINT("IopDereferenceNotify: NotifyHeader %p\n", NotifyHeader);
    PAGED_CODE();

    ASSERT(NotifyHeader);
    ASSERT(NotifyHeader->RefCount > 0);

    NotifyHeader->RefCount--;

    if (NotifyHeader->RefCount)
    {
        DPRINT("IopDereferenceNotify: NotifyHeader->RefCount %X\n", NotifyHeader->RefCount);
        return;
    }

    ASSERT(NotifyHeader->Unregistered);
    RemoveEntryList(&NotifyHeader->Link);

    ObDereferenceObject(NotifyHeader->DriverObject);

    if (NotifyHeader->EventCategory == EventCategoryTargetDeviceChange)
    {
        PTARGET_DEVICE_NOTIFY Notify = (PTARGET_DEVICE_NOTIFY)NotifyHeader;

        if (Notify->PhysicalDeviceObject)
        {
            ObDereferenceObject(Notify->PhysicalDeviceObject);
            Notify->PhysicalDeviceObject = NULL;
        }
    }

    if (NotifyHeader->OpaqueSession)
    {
        MmQuitNextSession(NotifyHeader->OpaqueSession);
        NotifyHeader->OpaqueSession = NULL;
    }

    ExFreePoolWithTag(NotifyHeader, '  pP');
}

NTSTATUS
NTAPI
PiDeferNotification(
    _In_ PPNP_NOTIFY_HEADER NotifyHeader)
{
    PPNP_DEFER_NOTIFY DeferNotification;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("PiDeferNotification: NotifyHeader %p\n", NotifyHeader);
    PAGED_CODE();

    KeAcquireGuardedMutex(&PiNotificationInProgressLock);
    if (!PiNotificationInProgress)
    {
        DPRINT1("PiNotificationInProgress is FALSE\n");
        ASSERT(IsListEmpty(&IopDeferredRegistrationList));
        goto Exit;
    }

    DeferNotification = ExAllocatePoolWithTag(PagedPool, sizeof(*DeferNotification), '  pP');
    if (!DeferNotification)
    {
        DPRINT1("PiDeferNotification: STATUS_INSUFFICIENT_RESOURCES\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    DeferNotification->NotifyHeader = NotifyHeader;
    NotifyHeader->Unregistered = TRUE;

    IopReferenceNotify(NotifyHeader);

    KeAcquireGuardedMutex(&IopDeferredRegistrationLock);
    InsertTailList(&IopDeferredRegistrationList, &DeferNotification->Link);
    KeReleaseGuardedMutex(&IopDeferredRegistrationLock);

Exit:

    KeReleaseGuardedMutex(&PiNotificationInProgressLock);

    return Status;
}

NTSTATUS
NTAPI
PiNotifyDriverCallback(
    _In_ PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
    _In_ PVOID NotificationStructure,
    _In_ PVOID Context,
    _In_ ULONG SessionId,
    _In_ PVOID OpaqueSession,
    _Out_ NTSTATUS* OutStatus)
{
    PEPROCESS Process;
    KAPC_STATE ApcState;
    ULONG CombinedApcDisable;
    KIRQL OldIrql;
    KIRQL NewIrql;
    NTSTATUS callbackStatus;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    if (!CallbackRoutine)
    {
        DPRINT1("PiNotifyDriverCallback: STATUS_INVALID_PARAMETER\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!NotificationStructure)
    {
        DPRINT1("PiNotifyDriverCallback: STATUS_INVALID_PARAMETER\n");
        return STATUS_INVALID_PARAMETER;
    }

    OldIrql = KeGetCurrentIrql();
    CombinedApcDisable = KeGetCurrentThread()->CombinedApcDisable;

    Process = (PEPROCESS)KeGetCurrentThread()->ApcState.Process;

    if (!OpaqueSession ||
        (Process->ProcessInSession && SessionId == PsGetCurrentProcessSessionId()))
    {
        ASSERT(!MmIsSessionAddress(CallbackRoutine) || OpaqueSession);

        DPRINT("PiNotifyDriverCallback: calling CallbackRoutine %p\n", CallbackRoutine);

        callbackStatus = CallbackRoutine(NotificationStructure, Context);
        if (OutStatus)
            *OutStatus = callbackStatus;

        if (!NT_SUCCESS(callbackStatus))
        {
            DPRINT1("PiNotifyDriverCallback: callbackStatus %X\n", callbackStatus);
            ASSERT(FALSE); // IoDbgBreakPointEx();
        }

        goto Exit;
    }

    ASSERT(MmIsSessionAddress(CallbackRoutine));

    Status = MmAttachSession(OpaqueSession, &ApcState);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PiNotifyDriverCallback: Status %X\n", Status);
        ASSERT(NT_SUCCESS(Status));
    }

    DPRINT("PiNotifyDriverCallback: CallbackRoutine %p, SessionId %X\n", CallbackRoutine, SessionId);

    callbackStatus = CallbackRoutine(NotificationStructure, Context);
    if (OutStatus)
        *OutStatus = callbackStatus;

    if (!NT_SUCCESS(callbackStatus))
    {
        DPRINT1("PiNotifyDriverCallback: callbackStatus %X\n", callbackStatus);
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    Status = 0;MmDetachSession(OpaqueSession, &ApcState); // TODO: add ret NTSTATUS for MmDetachSession().
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PiNotifyDriverCallback: Status %X\n", Status);
        ASSERT(NT_SUCCESS(Status));
    }

Exit:

    if (OldIrql != KeGetCurrentIrql())
    {
        NewIrql = KeGetCurrentIrql();
        DPRINT1("PiNotifyDriverCallback: CallbackRoutine %p, NewIrql %X, OldIrql %X\n", CallbackRoutine, NewIrql, OldIrql);
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    if (CombinedApcDisable != KeGetCurrentThread()->CombinedApcDisable)
    {
        DPRINT1("PiNotifyDriverCallback: CallbackRoutine %p, Current %X, Old %X\n", CallbackRoutine, KeGetCurrentThread()->CombinedApcDisable, CombinedApcDisable);
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    return Status;
}

VOID
NTAPI
IopNotifyDeviceClassChange(
    _In_ PPLUGPLAY_EVENT_BLOCK EventBlock,
    _In_ GUID* ClassGuid,
    _In_ PUNICODE_STRING SymbolicLinkName)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION InterfaceNotification;
    PDEVICE_INTERFACE_NOTIFY Notify;
    PLIST_ENTRY Head;
    PLIST_ENTRY Entry;
    NTSTATUS CallbackStatus;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopNotifyDeviceClassChange: EventBlock %p, SymbolicLink '%wZ'\n", EventBlock, SymbolicLinkName);

    InterfaceNotification.Version = 1;
    InterfaceNotification.Size = sizeof(InterfaceNotification);
    InterfaceNotification.Event = EventBlock->EventGuid;
    InterfaceNotification.InterfaceClassGuid = *ClassGuid;
    InterfaceNotification.SymbolicLinkName = SymbolicLinkName;

    KeAcquireGuardedMutex(&IopDeviceClassNotifyLock);

    Head = &IopDeviceClassNotifyList[HashGuid(ClassGuid)];

    for (Entry = Head->Flink; (Entry != Head); Entry = Entry->Flink)
    {
        Notify = CONTAINING_RECORD(Entry, DEVICE_INTERFACE_NOTIFY, Header.Link);

        if (Notify->Header.Unregistered)
            continue;

        if (!PiCompareGuid(&(Notify->Interface), ClassGuid))
            continue;

        IopReferenceNotify(&Notify->Header);
        KeReleaseGuardedMutex(&IopDeviceClassNotifyLock);

        Status = PiNotifyDriverCallback(Notify->Header.PnpNotificationRoutine,
                                        &InterfaceNotification,
                                        Notify->Header.Context,
                                        Notify->Header.SessionId,
                                        Notify->Header.OpaqueSession,
                                        &CallbackStatus);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopNotifyDeviceClassChange: Status %X\n", Status);
            ASSERT(NT_SUCCESS(Status));
        }

        KeAcquireGuardedMutex(&IopDeviceClassNotifyLock);
        IopDereferenceNotify(&Notify->Header);
    }

    KeReleaseGuardedMutex(&IopDeviceClassNotifyLock);
}

NTSTATUS
NTAPI
IopNotifyTargetDeviceChange(
    _In_ GUID* EventGuid,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PTARGET_DEVICE_CUSTOM_NOTIFICATION CustomStructure,
    _Out_ PDRIVER_OBJECT* OutDriverObject)
{
    TARGET_DEVICE_REMOVAL_NOTIFICATION TargetRemoval;
    PTARGET_DEVICE_NOTIFY Notification;
    PTARGET_DEVICE_NOTIFY notification;
    PVOID NotificationStructure;
    PDEVICE_NODE DeviceNode;
    PLIST_ENTRY Entry;
    BOOLEAN IsCancelled;
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(EventGuid != NULL);

    ObReferenceObject(DeviceObject);

    DeviceNode = IopGetDeviceNode(DeviceObject);
    ASSERT(DeviceNode != NULL);

    if (CustomStructure)
    {
        CustomStructure->Version = 1;
    }
    else
    {
        TargetRemoval.Version = 1;
        TargetRemoval.Size = sizeof(TargetRemoval);
        TargetRemoval.Event = *EventGuid;
    }

    KeAcquireGuardedMutex(&IopTargetDeviceNotifyLock);

    IsCancelled = RtlCompareMemory(EventGuid, &GUID_TARGET_DEVICE_REMOVE_CANCELLED, sizeof(GUID)) == sizeof(GUID);

    if (IsCancelled)
        Entry = DeviceNode->TargetDeviceNotify.Blink;
    else
        Entry = DeviceNode->TargetDeviceNotify.Flink;

    while (Entry != &DeviceNode->TargetDeviceNotify)
    {
        NTSTATUS status;

        Notification = CONTAINING_RECORD(Entry, TARGET_DEVICE_NOTIFY, Header.Link);
        DPRINT("IopNotifyTargetDeviceChange: DeviceObject %p, Notification %p\n", DeviceObject, Notification);

        if (Notification->Header.Unregistered)
        {
            if (IsCancelled)
                Entry = Entry->Blink;
            else
                Entry = Entry->Flink;

            continue;
        }

        IopReferenceNotify(&Notification->Header);
        KeReleaseGuardedMutex(&IopTargetDeviceNotifyLock);

        if (CustomStructure)
        {
            CustomStructure->FileObject = Notification->FileObject;
            NotificationStructure = CustomStructure;
        }
        else
        {
            TargetRemoval.FileObject = Notification->FileObject;
            NotificationStructure = &TargetRemoval;
        }

        DPRINT("IopNotifyTargetDeviceChange: Callback %p\n", Notification->Header.PnpNotificationRoutine);
        status = PiNotifyDriverCallback(Notification->Header.PnpNotificationRoutine,
                                        NotificationStructure,
                                        Notification->Header.Context,
                                        Notification->Header.SessionId,
                                        Notification->Header.OpaqueSession,
                                        &Status);
        if (!NT_SUCCESS(status))
        {
            DPRINT1("IopNotifyTargetDeviceChange: status %X\n", status);
            ASSERT(NT_SUCCESS(status));
            Status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(Status))
            goto Next;

        /* PiNotifyDriverCallback() return unsuccessful Status */
        DPRINT1("IopNotifyTargetDeviceChange: Status %X\n", Status);

        if (RtlCompareMemory(EventGuid, &GUID_TARGET_DEVICE_QUERY_REMOVE, 0x10) != 0x10)
        {
            ASSERT(Notification == (PVOID)CustomStructure);

            DPRINT1("IopNotifyTargetDeviceChange: Driver '%wZ', PnpNotificationRoutine %p\n", &Notification->Header.DriverObject->DriverName, Notification->Header.PnpNotificationRoutine);
            DPRINT1("IopNotifyTargetDeviceChange: Failed!!! Notification %p Status %X\n", Notification, Status);

            ASSERT(FALSE); // IoDbgBreakPointEx();
            goto Next;
        }

        ASSERT(Notification == (PVOID)&TargetRemoval);

        if (OutDriverObject)
            *OutDriverObject = Notification->Header.DriverObject;

        TargetRemoval.Event = GUID_TARGET_DEVICE_REMOVE_CANCELLED;
        notification = Notification;

        KeAcquireGuardedMutex(&IopTargetDeviceNotifyLock);

        do
        {
            Notification = CONTAINING_RECORD(Entry, TARGET_DEVICE_NOTIFY, Header.Link);

            if (Notification->Header.Unregistered)
            {
                Entry = Entry->Blink;
 
               if (Notification == notification)
                    IopDereferenceNotify(&notification->Header);

                continue;
            }

            IopReferenceNotify(&Notification->Header);
            KeReleaseGuardedMutex(&IopTargetDeviceNotifyLock);

            TargetRemoval.FileObject = Notification->FileObject;

            status = PiNotifyDriverCallback(Notification->Header.PnpNotificationRoutine,
                                            &TargetRemoval,
                                            Notification->Header.Context,
                                            Notification->Header.SessionId,
                                            Notification->Header.OpaqueSession,
                                            NULL);
            ASSERT(NT_SUCCESS(status));
            KeAcquireGuardedMutex(&IopTargetDeviceNotifyLock);

            Entry = Entry->Blink;

            IopDereferenceNotify(&Notification->Header);

            if (Notification == notification)
                IopDereferenceNotify(&notification->Header);

        }
        while (Entry != &DeviceNode->TargetDeviceNotify);

        goto Exit;

Next:
        KeAcquireGuardedMutex(&IopTargetDeviceNotifyLock);

        if (IsCancelled)
            Entry = Entry->Blink;
        else
            Entry = Entry->Flink;

        IopDereferenceNotify(&Notification->Header);
    }

    Status = STATUS_SUCCESS;

Exit:
    KeReleaseGuardedMutex(&IopTargetDeviceNotifyLock);
    ObDereferenceObject(DeviceObject);
    return Status;
}

VOID
NTAPI
IopProcessDeferredRegistrations(
    VOID)
{
    PPNP_DEFER_NOTIFY DeferNotification;
    PKGUARDED_MUTEX NotifyLock;
    PLIST_ENTRY Entry;

    DPRINT("IopProcessDeferredRegistrations()\n");
    PAGED_CODE();

    KeAcquireGuardedMutex(&IopDeferredRegistrationLock);

    while (!IsListEmpty(&IopDeferredRegistrationList))
    {
        Entry = RemoveHeadList(&IopDeferredRegistrationList);
        DeferNotification = CONTAINING_RECORD(Entry, PNP_DEFER_NOTIFY, Link);

        DPRINT("IopProcessDeferredRegistrations: DeferNotification %p\n", DeferNotification);

        NotifyLock = DeferNotification->NotifyHeader->NotifyLock;
        if (NotifyLock)
            KeAcquireGuardedMutex(NotifyLock);

        DeferNotification->NotifyHeader->Unregistered = FALSE;
        IopDereferenceNotify(DeferNotification->NotifyHeader);

        ExFreePool(DeferNotification);

        if (NotifyLock)
            KeReleaseGuardedMutex(NotifyLock);
    }

    KeReleaseGuardedMutex(&IopDeferredRegistrationLock);
}

/* PUBLIC FUNCTIONS **********************************************************/

/* unimplemented */
ULONG
NTAPI
IoPnPDeliverServicePowerNotification(ULONG VetoedPowerOperation OPTIONAL,
                                     ULONG PowerNotification,
                                     ULONG Unknown OPTIONAL,
                                     BOOLEAN Synchronous)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return 0;
}

NTSTATUS
NTAPI
IoRegisterPlugPlayNotification(
    _In_ IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    _In_ ULONG EventCategoryFlags,
    _In_ PVOID EventCategoryData OPTIONAL,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
    _In_ PVOID Context,
    _Out_ PVOID* NotificationEntry)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION InterfaceNotification;
    PHARDWARE_PROFILE_NOTIFY HwProfileNotify;
    PDEVICE_INTERFACE_NOTIFY IntarfaceNotify;
    PTARGET_DEVICE_NOTIFY TargetDeviceNotify;
    PSETUP_NOTIFY_DATA SetupNotify;
    UNICODE_STRING DestinationString;
    PDEVICE_NODE deviceNode;
    PWSTR SymbolicLinkList;
    PVOID Session;
    NTSTATUS Status;

    DPRINT1("IoRegisterPlugPlayNotification: %X, %X, %p, %p\n", EventCategory, EventCategoryFlags, EventCategoryData, DriverObject);
    PAGED_CODE();

    ASSERT(NotificationEntry);
    *NotificationEntry = NULL;

    Status = ObReferenceObjectByPointer(DriverObject, 0, IoDriverObjectType, KernelMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoRegisterPlugPlayNotification: Status %X\n", Status);
        return Status;
    }

    if (EventCategory == EventCategoryReserved) // Setup Notify
    {
        if (!ExpInTextModeSetup)
        {
            *NotificationEntry = NULL;
            goto Exit;
        }

        ASSERT(IopSetupNotifyData == NULL);
        ASSERT(MmIsSessionAddress((PVOID)CallbackRoutine) == FALSE);
        ASSERT(MmGetSessionId(PsGetCurrentProcess()) == 0);

        SetupNotify = ExAllocatePoolWithTag(PagedPool, sizeof(SETUP_NOTIFY_DATA), '  pP');
        if (!SetupNotify)
        {
            DPRINT1("IoRegisterPlugPlayNotification: STATUS_INSUFFICIENT_RESOURCES\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit;
        }

        if (MmIsSessionAddress(CallbackRoutine))
            Session = MmGetSessionById(SetupNotify->Header.SessionId);
        else
            Session = NULL;

        InitializeListHead(&SetupNotify->Header.Link);

        SetupNotify->Header.EventCategory = EventCategoryReserved;
        SetupNotify->Header.SessionId = MmGetSessionId(PsGetCurrentProcess());
        SetupNotify->Header.OpaqueSession = Session;
        SetupNotify->Header.PnpNotificationRoutine = CallbackRoutine;
        SetupNotify->Header.Context = Context;
        SetupNotify->Header.DriverObject = DriverObject;
        SetupNotify->Header.RefCount = 1;
        SetupNotify->Header.Unregistered = FALSE;
        SetupNotify->Header.NotifyLock = NULL;

        IopSetupNotifyData = SetupNotify;

        *NotificationEntry = NULL;
        goto Exit;
    }

    if (EventCategory == EventCategoryHardwareProfileChange)
    {
        HwProfileNotify = ExAllocatePoolWithTag(PagedPool, sizeof(HARDWARE_PROFILE_NOTIFY), '  pP');
        if (!HwProfileNotify)
        {
            DPRINT1("IoRegisterPlugPlayNotification: STATUS_INSUFFICIENT_RESOURCES\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit;
        }

        if (MmIsSessionAddress(CallbackRoutine))
            Session = MmGetSessionById(HwProfileNotify->Header.SessionId);
        else
            Session = NULL;

        HwProfileNotify->Header.EventCategory = EventCategoryHardwareProfileChange;
        HwProfileNotify->Header.SessionId = MmGetSessionId(PsGetCurrentProcess());
        HwProfileNotify->Header.OpaqueSession = Session;
        HwProfileNotify->Header.PnpNotificationRoutine = CallbackRoutine;
        HwProfileNotify->Header.Context = Context;
        HwProfileNotify->Header.DriverObject = DriverObject;
        HwProfileNotify->Header.RefCount = 1;
        HwProfileNotify->Header.Unregistered = FALSE;
        HwProfileNotify->Header.NotifyLock = &IopHwProfileNotifyLock;

        Status = PiDeferNotification(&HwProfileNotify->Header);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoRegisterPlugPlayNotification: Status %X\n", Status);
            ExFreePoolWithTag(HwProfileNotify, '  pP');
            goto Exit;
        }

        KeAcquireGuardedMutex(&IopHwProfileNotifyLock);
        InsertTailList(&IopProfileNotifyList, &HwProfileNotify->Header.Link);
        KeReleaseGuardedMutex(&IopHwProfileNotifyLock);

        *NotificationEntry = HwProfileNotify;
        goto Exit;
    }

    if (EventCategory == EventCategoryDeviceInterfaceChange)
    {
        NTSTATUS CallbackStatus;
        NTSTATUS sts;
        PWSTR Str;

        ASSERT(EventCategoryData);

        IntarfaceNotify = ExAllocatePoolWithTag(PagedPool, sizeof(DEVICE_INTERFACE_NOTIFY), '  pP');
        if (!IntarfaceNotify)
        {
            DPRINT("IoRegisterPlugPlayNotification: STATUS_INSUFFICIENT_RESOURCES\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit;
        }

        if (MmIsSessionAddress(CallbackRoutine))
            Session = MmGetSessionById(IntarfaceNotify->Header.SessionId);
        else
            Session = NULL;

        IntarfaceNotify->Header.EventCategory = EventCategoryDeviceInterfaceChange;
        IntarfaceNotify->Header.SessionId = MmGetSessionId(PsGetCurrentProcess());
        IntarfaceNotify->Header.OpaqueSession = Session;
        IntarfaceNotify->Header.PnpNotificationRoutine = CallbackRoutine;
        IntarfaceNotify->Header.Context = Context;
        IntarfaceNotify->Header.DriverObject = DriverObject;
        IntarfaceNotify->Header.RefCount = 1;
        IntarfaceNotify->Header.Unregistered = FALSE;
        IntarfaceNotify->Header.NotifyLock = &IopDeviceClassNotifyLock;

        RtlCopyMemory(&IntarfaceNotify->Interface, EventCategoryData, sizeof(IntarfaceNotify->Interface));

        Status = PiDeferNotification(&IntarfaceNotify->Header);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoRegisterPlugPlayNotification: Status %X\n", Status);
            ExFreePoolWithTag(IntarfaceNotify, '  pP');
            goto Exit;
        }

        KeAcquireGuardedMutex(&IopDeviceClassNotifyLock);
        InsertTailList(&IopDeviceClassNotifyList[HashGuid(&IntarfaceNotify->Interface)], &IntarfaceNotify->Header.Link);
        KeReleaseGuardedMutex(&IopDeviceClassNotifyLock);

        if (!(EventCategoryFlags & 1))
        {
            DPRINT1("IoRegisterPlugPlayNotification: !(EventCategoryFlags & 1)\n");
            *NotificationEntry = IntarfaceNotify;
            goto Exit;
        }

        InterfaceNotification.Version = 1;
        InterfaceNotification.Size = sizeof(InterfaceNotification);
        InterfaceNotification.Event = GUID_DEVICE_INTERFACE_ARRIVAL;
        InterfaceNotification.InterfaceClassGuid = IntarfaceNotify->Interface;

        Status = IoGetDeviceInterfaces(&IntarfaceNotify->Interface, NULL, 0, &SymbolicLinkList);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoRegisterPlugPlayNotification: Status %X\n", Status);
            goto ErrorExit;
        }

        for (Str = SymbolicLinkList;
             *Str;
             Str += ((DestinationString.Length / sizeof(WCHAR)) + 1))
        {
            RtlInitUnicodeString(&DestinationString, Str);
            InterfaceNotification.SymbolicLinkName = &DestinationString;

            DPRINT("IoRegisterPlugPlayNotification: CallbackRoutine %X\n", CallbackRoutine);

            sts = PiNotifyDriverCallback(CallbackRoutine,
                                         &InterfaceNotification,
                                         Context,
                                         IntarfaceNotify->Header.SessionId,
                                         IntarfaceNotify->Header.OpaqueSession,
                                         &CallbackStatus);
            if (!NT_SUCCESS(sts))
            {
                DPRINT1("IoRegisterPlugPlayNotification: sts %X\n", sts);
                ASSERT(NT_SUCCESS(sts));
            }
        }

        ExFreePool(SymbolicLinkList);

        *NotificationEntry = IntarfaceNotify;
        goto Exit;
    }

    if (EventCategory == EventCategoryTargetDeviceChange)
    {
        ASSERT(EventCategoryData);

        TargetDeviceNotify = ExAllocatePoolWithTag(PagedPool, sizeof(TARGET_DEVICE_NOTIFY), '  pP');
        if (!TargetDeviceNotify)
        {
            DPRINT1("IoRegisterPlugPlayNotification: STATUS_INSUFFICIENT_RESOURCES\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit;
        }

        Status = IopGetRelatedTargetDevice((PFILE_OBJECT)EventCategoryData, &deviceNode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoRegisterPlugPlayNotification: Status %X\n", Status);
            ExFreePoolWithTag(TargetDeviceNotify, '  pP');
            goto Exit;
        }

        if (MmIsSessionAddress(CallbackRoutine))
            Session = MmGetSessionById(TargetDeviceNotify->Header.SessionId);
        else
            Session = NULL;

        TargetDeviceNotify->Header.EventCategory = EventCategoryTargetDeviceChange;
        TargetDeviceNotify->Header.SessionId = MmGetSessionId(PsGetCurrentProcess());
        TargetDeviceNotify->Header.OpaqueSession = Session;
        TargetDeviceNotify->Header.PnpNotificationRoutine = CallbackRoutine;
        TargetDeviceNotify->Header.Context = Context;
        TargetDeviceNotify->Header.DriverObject = DriverObject;
        TargetDeviceNotify->Header.RefCount = 1;
        TargetDeviceNotify->Header.Unregistered = FALSE;
        TargetDeviceNotify->Header.NotifyLock = &IopTargetDeviceNotifyLock;

        TargetDeviceNotify->FileObject = EventCategoryData;

        ASSERT(deviceNode->PhysicalDeviceObject);
        TargetDeviceNotify->PhysicalDeviceObject = deviceNode->PhysicalDeviceObject;

        Status = PiDeferNotification(&TargetDeviceNotify->Header);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoRegisterPlugPlayNotification: Status %X\n", Status);
            ExFreePoolWithTag(TargetDeviceNotify, '  pP');
            goto Exit;
        }

        KeAcquireGuardedMutex(&IopTargetDeviceNotifyLock);
        InsertTailList(&deviceNode->TargetDeviceNotify, &TargetDeviceNotify->Header.Link);
        KeReleaseGuardedMutex(&IopTargetDeviceNotifyLock);

        *NotificationEntry = TargetDeviceNotify;
        goto Exit;
    }

    DPRINT1("IoRegisterPlugPlayNotification: Unknown EventCategory %X\n", EventCategory);
    ASSERT(FALSE); // IoDbgBreakPointEx();
    Status = STATUS_INVALID_PARAMETER;

Exit:
    if (NT_SUCCESS(Status))
        return Status;

ErrorExit:

    ObDereferenceObject(DriverObject);

    DPRINT("IoRegisterPlugPlayNotification: return %X\n", Status);
    return Status;
}

/* halfplemented */
NTSTATUS
NTAPI
IoUnregisterPlugPlayNotification(
    _In_ PVOID NotificationEntry)
{
    PPNP_NOTIFY_HEADER NotifyHeader = NotificationEntry;
    PKGUARDED_MUTEX Lock = NotifyHeader->NotifyLock;
    BOOLEAN wasDeferred = FALSE;

    DPRINT("IoUnregisterPlugPlayNotification: NotificationEntry %p\n", NotificationEntry);

    PAGED_CODE();
    ASSERT(NotificationEntry);

    KeAcquireGuardedMutex(&PiNotificationInProgressLock);

    if (PiNotificationInProgress)
    {
        DPRINT1("IoUnregisterPlugPlayNotification: PiNotificationInProgress is TRUE\n");

        KeAcquireGuardedMutex(&IopDeferredRegistrationLock);

        if (!IsListEmpty(&IopDeferredRegistrationList))
        {
            DPRINT("IoUnregisterPlugPlayNotification: FIXME! Head %p\n", &IopDeferredRegistrationList);
            ASSERT(FALSE); // IoDbgBreakPointEx();

        }

        KeReleaseGuardedMutex(&IopDeferredRegistrationLock);
    }
    else
    {
        ASSERT(IsListEmpty(&IopDeferredRegistrationList));
    }

    KeReleaseGuardedMutex(&PiNotificationInProgressLock);

    if (Lock)
        KeAcquireGuardedMutex(Lock);

    ASSERT(wasDeferred == NotifyHeader->Unregistered);

    if (!NotifyHeader->Unregistered || wasDeferred)
    {
        NotifyHeader->Unregistered = TRUE;
        IopDereferenceNotify(NotifyHeader);
    }

    if (Lock)
        KeReleaseGuardedMutex(Lock);

    DPRINT("IoUnregisterPlugPlayNotification: return STATUS_SUCCESS\n");
    return STATUS_SUCCESS;
}

/* EOF */

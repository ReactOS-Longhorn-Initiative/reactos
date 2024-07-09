
/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

BOOLEAN PiNotificationInProgress = FALSE;

extern PPNP_DEVICE_EVENT_LIST PpDeviceEventList;
extern KGUARDED_MUTEX PiNotificationInProgressLock;

extern KEVENT PiEventQueueEmpty;
extern BOOLEAN PpPnpShuttingDown;

/* FUNCTIONS *****************************************************************/

PVOID
NTAPI
PiAllocateCriticalMemory(
    _In_ PIP_TYPE_REMOVAL_DEVICE DeleteType,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag)
{
    PVOID Block;
    LARGE_INTEGER Interval;

    DPRINT("PiAllocateCriticalMemory: DeleteType %X, NumberOfBytes %X\n", DeleteType, NumberOfBytes);
    PAGED_CODE();

    ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

    while (TRUE)
    {
        Block = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);

        if (Block || DeleteType == PipQueryRemove || DeleteType == PipEject)
            break;

        Interval.QuadPart = (-10000ll * 1); // 1 msec
        KeDelayExecutionThread(KernelMode, FALSE, &Interval);
    }

    return Block;
}

VOID
NTAPI
PpCompleteDeviceEvent(
    _In_ PPNP_DEVICE_EVENT_ENTRY EventEntry,
    _In_ NTSTATUS Status)
{
    DPRINT("PpCompleteDeviceEvent: EventEntry %p, Status %X\n", EventEntry, Status);
    PAGED_CODE();

    if (EventEntry->CallerEvent)
    {
        *EventEntry->Data.Result = Status;
        KeSetEvent(EventEntry->CallerEvent, IO_NO_INCREMENT, FALSE);
    }

    if (EventEntry->Callback)
    {
        DPRINT1("PpCompleteDeviceEvent: FIXME Callback\n");
        ASSERT(FALSE);
        //EventEntry->Callback(EventEntry->Context);
    }

    if (EventEntry->Data.DeviceObject)
        ObDereferenceObject(EventEntry->Data.DeviceObject);

    ExFreePoolWithTag(EventEntry, 'EEpP');
}

NTSTATUS
NTAPI
PiResizeTargetDeviceBlock(
    _Out_ PPNP_DEVICE_EVENT_ENTRY*  pEventEntry,
    _In_ PIP_TYPE_REMOVAL_DEVICE RemovalType,
    _In_ PRELATION_LIST RelationsList,
    _In_ BOOLEAN IsNoInstance)
{
    PPNP_DEVICE_EVENT_ENTRY NewEventEntry;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    PWCHAR pChar;
    ULONG SizeHead;
    ULONG CurrentSize;
    ULONG NewSize;
    ULONG Marker;
    UCHAR OutIsDirectDescendant;
  
    DPRINT("PiResizeTargetDeviceBlock: EventEntry %p\n", *pEventEntry);
    PAGED_CODE();

    if (!RelationsList)
    {
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_SUCCESS;
    }

    SizeHead = (sizeof(**pEventEntry) - sizeof(PLUGPLAY_EVENT_BLOCK));

    CurrentSize = (SizeHead + (*pEventEntry)->Data.TotalSize);
    NewSize = (CurrentSize - wcslen((*pEventEntry)->Data.TargetDevice.DeviceIds) * sizeof(WCHAR));

    DPRINT("PiResizeTargetDeviceBlock: CurrentSize %X NewSize %X\n", CurrentSize, NewSize);

    Marker = 0;
    while (IopEnumerateRelations(RelationsList, &Marker, &DeviceObject, &OutIsDirectDescendant, NULL, FALSE))
    {
        if (!IsNoInstance || OutIsDirectDescendant)
        {
            DeviceNode = IopGetDeviceNode(DeviceObject);
            if (DeviceNode)
            {
                if (DeviceNode->InstancePath.Length)
                    NewSize += (DeviceNode->InstancePath.Length + sizeof(WCHAR));
            }
        }
    }

    DPRINT("PiResizeTargetDeviceBlock: NewSize %X CurrentSize %X\n", NewSize, CurrentSize);

    ASSERT(NewSize >= CurrentSize);

    if (NewSize == CurrentSize)
        return STATUS_SUCCESS;

    if (NewSize < CurrentSize)
        NewSize = CurrentSize;

    NewEventEntry = PiAllocateCriticalMemory(RemovalType, PagedPool, NewSize, 'EEpP');
    if (!NewEventEntry)
    {
        DPRINT1("PiResizeTargetDeviceBlock: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(NewEventEntry, NewSize);
    RtlCopyMemory(NewEventEntry, *pEventEntry, CurrentSize);

    NewEventEntry->Data.TotalSize = (NewSize - SizeHead);
    pChar = (NewEventEntry->Data.TargetDevice.DeviceIds + wcslen((*pEventEntry)->Data.TargetDevice.DeviceIds) + 1);

    Marker = 0;
    while (IopEnumerateRelations(RelationsList, &Marker, &DeviceObject, &OutIsDirectDescendant, NULL, FALSE))
    {
        if (DeviceObject != NewEventEntry->Data.DeviceObject && (!IsNoInstance || OutIsDirectDescendant))
        {
            DeviceNode = IopGetDeviceNode(DeviceObject);

            if (DeviceNode && DeviceNode->InstancePath.Length)
            {
                RtlCopyMemory(pChar, DeviceNode->InstancePath.Buffer, DeviceNode->InstancePath.Length);
                pChar += ((DeviceNode->InstancePath.Length / sizeof(WCHAR)) + 1);
            }
        }
    }
 
    *pChar = UNICODE_NULL;

    ExFreePool(*pEventEntry);
    *pEventEntry = NewEventEntry;

    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
PipIsBeingRemovedSafely(
    _In_ PDEVICE_NODE DeviceNode)
{
    DPRINT("PipIsBeingRemovedSafely: DeviceNode %p, State %X\n", DeviceNode, DeviceNode->State);
    PAGED_CODE();

    ASSERT(DeviceNode->State == DeviceNodeAwaitingQueuedDeletion);

    if (((DeviceNode->CapabilityFlags) & 0x0200)) // SurpriseRemovalOK
        return TRUE;

    if (DeviceNode->PreviousState == DeviceNodeStarted ||
        DeviceNode->PreviousState == DeviceNodeStopped ||
        DeviceNode->PreviousState == DeviceNodeStartPostWork ||
        DeviceNode->PreviousState == DeviceNodeRestartCompletion)
    {
        return FALSE;
    }

    return TRUE;
}
VOID
NTAPI
PiBuildUnsafeRemovalDeviceBlock(
    _In_ PPNP_DEVICE_EVENT_ENTRY EventEntry,
    _In_ PRELATION_LIST RelationsList,
    _Out_ PPNP_DEVICE_EVENT_ENTRY* OutEventEntry)
{
    PPNP_DEVICE_EVENT_ENTRY UnsafeEventEntry;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    PWCHAR pChar;
    ULONG Marker;
    ULONG SizeHead;
    ULONG SizeDataBlockHead;
    ULONG SizeInstances;
    ULONG NewSize;
    BOOLEAN OutIsDirectDescendant;

    DPRINT("PiBuildUnsafeRemovalDeviceBlock: EventEntry %p, RelationsList %p\n", EventEntry, RelationsList);
    PAGED_CODE();

    *OutEventEntry = NULL;

    if (!RelationsList)
        return;

    SizeInstances = 0;
    Marker = 0;

    while (IopEnumerateRelations(RelationsList, &Marker, &DeviceObject, &OutIsDirectDescendant, NULL, FALSE))
    {
        if (OutIsDirectDescendant)
        {
            DeviceNode = IopGetDeviceNode(DeviceObject);
            if (DeviceNode)
            {
                if (!PipIsBeingRemovedSafely(DeviceNode))
                {
                    if (DeviceNode->InstancePath.Length)
                        SizeInstances += DeviceNode->InstancePath.Length + sizeof(WCHAR);
                }
            }
        }
    }

    if (!SizeInstances)
        return;

    SizeHead = (sizeof(*UnsafeEventEntry) - sizeof(UnsafeEventEntry->Data));

    NewSize = SizeHead + SizeInstances + sizeof(WCHAR);

    UnsafeEventEntry = ExAllocatePoolWithTag(PagedPool, NewSize, 'EEpP');
    if (!UnsafeEventEntry)
    {
        DPRINT1("PiBuildUnsafeRemovalDeviceBlock: Allocate failed\n");
        return;
    }

    RtlZeroMemory(UnsafeEventEntry, NewSize);
    RtlCopyMemory(UnsafeEventEntry, EventEntry, SizeHead);

    pChar = (PWCHAR)((int)UnsafeEventEntry + SizeHead);

    SizeDataBlockHead = (sizeof(UnsafeEventEntry->Data) - sizeof(UnsafeEventEntry->Data));
    UnsafeEventEntry->Data.TotalSize = (SizeDataBlockHead + SizeInstances + sizeof(WCHAR));

    Marker = 0;
    while (IopEnumerateRelations(RelationsList, &Marker, &DeviceObject, &OutIsDirectDescendant, NULL, FALSE))
    {
        if (OutIsDirectDescendant)
        {
            DeviceNode = IopGetDeviceNode(DeviceObject);
            if (DeviceNode)
            {
                if (!PipIsBeingRemovedSafely(DeviceNode))
                {
                    if (DeviceNode->InstancePath.Length)
                    {
                        RtlCopyMemory(pChar, DeviceNode->InstancePath.Buffer, DeviceNode->InstancePath.Length);
                        pChar += ((DeviceNode->InstancePath.Length / sizeof(WCHAR)) + 1);
                    }
                }
            }
        }
    }

    *pChar = UNICODE_NULL;
    *OutEventEntry = UnsafeEventEntry;
}

NTSTATUS
NTAPI
IopRemoveIndirectRelationsFromList(
    _In_ PRELATION_LIST RelationsList)
{
    PRELATION_LIST_ENTRY RelationEntry;
    ULONG Level;
    LONG ix;
  
    DPRINT("IopRemoveIndirectRelationsFromList: RelationsList %p\n", RelationsList);
    PAGED_CODE();

    for (Level = RelationsList->FirstLevel;
         Level <= RelationsList->MaxLevel;
         Level++)
    {
        RelationEntry = RelationsList->Entries[Level - RelationsList->FirstLevel];
        if (!RelationEntry)
            continue;

        ix = (RelationEntry->Count - 1);

        for (; ix >= 0; ix--, RelationsList->Count--)
        {
            if ((ULONG_PTR)RelationEntry->Devices[ix] & 2)
                continue;

            ObDereferenceObject((PVOID)((ULONG_PTR)RelationEntry->Devices[ix] & 0xFFFFFFFC));

            if ((ULONG_PTR)RelationEntry->Devices[ix] & 1)
                RelationsList->TagCount--;

            if (ix < (LONG)(RelationEntry->Count - 1))
            {
                RtlMoveMemory(&RelationEntry->Devices[ix],
                              &RelationEntry->Devices[ix + 1],
                              ((RelationEntry->Count - (ix + 1)) * sizeof(*RelationEntry->Devices)));
            }

            if (RelationEntry->Count-- == 1)
            {
                RelationsList->Entries[Level - RelationsList->FirstLevel] = NULL;
                ExFreePool(RelationEntry);
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiProcessQueryRemoveAndEject(
    _In_ PPNP_DEVICE_EVENT_ENTRY* pEventEntry)
{
    PPNP_DEVICE_EVENT_ENTRY EventEntry = *pEventEntry;
    PPNP_DEVICE_EVENT_ENTRY UnsafeRemovalEntry = NULL;
    PIP_TYPE_REMOVAL_DEVICE RemovalType;
    UNICODE_STRING VetoString;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT EnumDevice;
    PDEVICE_OBJECT* RemovalDevices;
    PDEVICE_NODE DeviceNode;
    PDEVICE_NODE RelatedDeviceNode;
    PRELATION_LIST RelationsList;
    PNP_VETO_TYPE Reason;
    PVOID DockInterface = NULL;
    PVOID VetoBuffer;
    ULONG RelationsCount;
    ULONG Marker;
    ULONG ix;
    BOOLEAN IsTransitionHwProfile = FALSE;
    BOOLEAN IsDirectDescendant;
    BOOLEAN IsForceDescendant;
    BOOLEAN WarmEjectSupported;
    BOOLEAN EjectSupported;
    BOOLEAN ResizeParam;
    NTSTATUS Status;

    DPRINT("PiProcessQueryRemoveAndEject: EventEntry %p\n", EventEntry);
    PAGED_CODE();

    DeviceObject = EventEntry->Data.DeviceObject;
    DeviceNode = IopGetDeviceNode(DeviceObject);

    PpDevNodeLockTree(1);

    if (PiCompareGuid(&EventEntry->Data.EventGuid, &GUID_DEVICE_EJECT))
    {
        RemovalType = PipEject;
    }
    else if (EventEntry->Data.Flags & 4)
    {
        if (DeviceNode->Flags & DNF_ENUMERATED)
        {
            ASSERT(DeviceNode->State == DeviceNodeAwaitingQueuedRemoval);

            if (DeviceNode->PreviousState == DeviceNodeStarted ||
                DeviceNode->PreviousState == DeviceNodeStopped ||
                DeviceNode->PreviousState == DeviceNodeStartPostWork ||
                DeviceNode->PreviousState == DeviceNodeRestartCompletion)
            {
                RemovalType = PipRemoveFailed;
            }
            else
            {
                RemovalType = PipRemoveFailedNotStarted;
            }
        }
        else
        {
            ASSERT(DeviceNode->State == DeviceNodeAwaitingQueuedDeletion);

            if (DeviceNode->PreviousState == DeviceNodeStarted ||
                DeviceNode->PreviousState == DeviceNodeStopped ||
                DeviceNode->PreviousState == DeviceNodeStartPostWork ||
                DeviceNode->PreviousState == DeviceNodeRestartCompletion)
            {
                RemovalType = PipSurpriseRemove;
            }
            else
            {
                RemovalType = PipRemove;
            }
        }
    }
    else
    {
        RemovalType = PipQueryRemove;
    }

    DPRINT("PiProcessQueryRemoveAndEject: Removal type %X\n", RemovalType);

    if ((RemovalType == PipQueryRemove || RemovalType == PipEject) &&
        (DeviceNode->Flags & DNF_LEGACY_DRIVER))
    {
        DPRINT1("PiProcessQueryRemoveAndEject: Vetoed by legacy driver\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

        PpDevNodeUnlockTree(1);
        return STATUS_PLUGPLAY_QUERY_VETOED;
    }

    if (RemovalType == PipQueryRemove &&
        EventEntry->Argument == CM_PROB_DISABLED &&
        DeviceNode->DisableableDepends > 0)
    {
        DPRINT1("PiProcessQueryRemoveAndEject: Device is non-disableable\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

        PpDevNodeUnlockTree(1);
        return STATUS_PLUGPLAY_QUERY_VETOED;
    }

    VetoBuffer = PiAllocateCriticalMemory(RemovalType, PagedPool, 0x400, 'rcpP');
    if (!VetoBuffer)
    {
        DPRINT("PiProcessQueryRemoveAndEject: Vetoed due to failure to allocate VetoBuffer\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

        PpDevNodeUnlockTree(1);
        return STATUS_PLUGPLAY_QUERY_VETOED;
    }

    Reason = 0;

    VetoString.Length = 0;
    VetoString.MaximumLength = 0x200;
    VetoString.Buffer = VetoBuffer;

    if (RemovalType == PipEject)
    {
        if (DeviceNode->Flags & DNF_LOCKED_FOR_EJECT)
        {
            DPRINT("PiProcessQueryRemoveAndEject: Device already being ejected\n");
            ExFreePool(VetoBuffer);
            PpDevNodeUnlockTree(1);
            return STATUS_SUCCESS;
        }

        if (EventEntry->Data.Flags & 4)
        {
            DPRINT1("PiProcessQueryRemoveAndEject: Kernel initiated eject vetoed by user mode\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();//Status = PiNotifyUserModeKernelInitiatedEject(..);

            Status = STATUS_PLUGPLAY_QUERY_VETOED;
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("PiProcessQueryRemoveAndEject: Status %X\n", Status);
                ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

                ExFreePool(VetoBuffer);
                PpDevNodeUnlockTree(1);
                return STATUS_PLUGPLAY_QUERY_VETOED;
            }
        }

        if (DeviceNode->DockInfo.DockStatus == DOCK_DEPARTING ||
            DeviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED)
        {
            DPRINT("PiProcessQueryRemoveAndEject: Dock already being ejected\n");
            ExFreePool(VetoBuffer);
            PpDevNodeUnlockTree(1);
            return STATUS_SUCCESS;
        }

        if (!(DeviceNode->CapabilityFlags & 0x10))
        {
            DPRINT1("PiProcessQueryRemoveAndEject: Device not removable\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

            ExFreePool(VetoBuffer);
            PpDevNodeUnlockTree(1);
            return STATUS_PLUGPLAY_QUERY_VETOED;
        }
    }
    else if (RemovalType == PipQueryRemove && !PipAreDriversLoaded(DeviceNode))
    {
        DPRINT1("PiProcessQueryRemoveAndEject: PipQueryRemove\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();

        Status = STATUS_SUCCESS;
        if (DeviceNode->State == DeviceNodeInitialized || DeviceNode->State == DeviceNodeRemoved)
        {
            if (DeviceNode->Flags & (DNF_HAS_PRIVATE_PROBLEM | DNF_HAS_PROBLEM))
            {
                if (!PipIsProblemReadonly(DeviceNode->Problem))
                    PipClearDevNodeProblem(DeviceNode);
            }

            if (DeviceNode->Flags & (DNF_HAS_PRIVATE_PROBLEM | DNF_HAS_PROBLEM))
            {
                if (!(EventEntry->Data.Flags & 2))
                    Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (EventEntry->Data.Flags & 2)
                    PipSetDevNodeProblem(DeviceNode, EventEntry->Argument);
                else
                    IopRestartDeviceNode(DeviceNode);
            }
        }

        PpDevNodeUnlockTree(1);
        ExFreePool(VetoBuffer);
        return Status;
    }

    Status = IopBuildRemovalRelationList(DeviceObject, RemovalType, &Reason, &VetoString, &RelationsList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PiProcessQueryRemoveAndEject: Status %X\n", Status);

        DPRINT1("PiProcessQueryRemoveAndEject: Failed to build removal relations\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

        ExFreePool(VetoBuffer);
        PpDevNodeUnlockTree(1);
        return STATUS_PLUGPLAY_QUERY_VETOED;
    }

    ASSERT(RelationsList != NULL);
    ASSERT(!RelationsList->TagCount);

    RelationsCount = RelationsList->Count;
    DPRINT("PiProcessQueryRemoveAndEject: RelationsCount %X\n", RelationsCount);

    RemovalDevices = PiAllocateCriticalMemory(RemovalType, NonPagedPool, (RelationsCount * sizeof(PDEVICE_OBJECT)), 'rcpP');
    if (!RemovalDevices)
    {
        DPRINT1("PiProcessQueryRemoveAndEject: STATUS_INSUFFICIENT_RESOURCES\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        RelationsCount = 0;
        Marker = 0;

        while (IopEnumerateRelations(RelationsList, &Marker, &EnumDevice, &IsDirectDescendant, NULL, TRUE))
        {
            if (!IsDirectDescendant && (RemovalType != PipEject) && (RemovalType != PipQueryRemove))
                continue;

            RelatedDeviceNode = IopGetDeviceNode(EnumDevice);

            ASSERT(RelatedDeviceNode->DockInfo.DockStatus != DOCK_ARRIVING);

            if (RemovalType != PipRemove && RemovalType != PipQueryRemove)
            {
                if (RelatedDeviceNode->DockInfo.DockStatus == DOCK_QUIESCENT)
                {
                    IsTransitionHwProfile = TRUE;
                }
                else if (RelatedDeviceNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE)
                {
                    DPRINT1("PiProcessQueryRemoveAndEject: FIXME\n");
                    ASSERT(FALSE); // IoDbgBreakPointEx();
                }
            }

            if (RemovalType == PipQueryRemove || RemovalType == PipEject)
            {
                if (RelatedDeviceNode->Flags & DNF_LEGACY_DRIVER)
                {
                    DPRINT1("PiProcessQueryRemoveAndEject: Vetoed by legacy driver relation\n");
                    ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                if (RelatedDeviceNode->State == DeviceNodeRemovePendingCloses)
                {
                    DPRINT1("PiProcessQueryRemoveAndEject: Vetoed due to device in DeviceNodeRemovePendingCloses\n");
                    ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }
            }

            RemovalDevices[RelationsCount++] = EnumDevice;
        }
    }

    if (NT_SUCCESS(Status))
    {
        if (RemovalType == PipSurpriseRemove ||
            RemovalType == PipRemoveFailed ||
            RemovalType == PipRemoveFailedNotStarted ||
            RemovalType == PipRemove)
        {
            ResizeParam = TRUE;
        }
        else
        {
            ResizeParam = FALSE;
        }

        Status = PiResizeTargetDeviceBlock(pEventEntry, RemovalType, RelationsList, ResizeParam);

        EventEntry = *pEventEntry;

        if (RemovalType == PipSurpriseRemove)
            PiBuildUnsafeRemovalDeviceBlock(EventEntry, RelationsList, &UnsafeRemovalEntry);
    }

    if (!NT_SUCCESS(Status))
    {
        IopFreeRelationList(RelationsList);

        if (RemovalDevices)
            ExFreePool(RemovalDevices);

        ExFreePool(VetoBuffer);

        DPRINT1("PiProcessQueryRemoveAndEject: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();//PiFinalizeVetoedRemove(..);

        PpDevNodeUnlockTree(1);
        return Status;
    }

    if (IsTransitionHwProfile)
    {
        DPRINT1("PiProcessQueryRemoveAndEject: FIXME PpProfileBeginHardwareProfileTransition()\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//PpProfileBeginHardwareProfileTransition(..);

        for (ix = (RelationsCount - 1); ix >= 0; ix--)
        {
            EnumDevice = RemovalDevices[ix];
            RelatedDeviceNode = IopGetDeviceNode(EnumDevice);

            ASSERT(RelatedDeviceNode->DockInfo.DockStatus != DOCK_ARRIVING);

            if (RelatedDeviceNode->DockInfo.DockStatus == DOCK_QUIESCENT)
            {
                DPRINT1("PiProcessQueryRemoveAndEject: FIXME PpProfileIncludeInHardwareProfileTransition()\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();//PpProfileIncludeInHardwareProfileTransition(..);
            }
        }

        ASSERT(RemovalType != PipQueryRemove && RemovalType != PipRemoveFailed);

        if (RemovalType == PipEject)
        {
            DPRINT1("PiProcessQueryRemoveAndEject: FIXME IoGetLegacyVetoList()\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();//Status = IoGetLegacyVetoList(..)
        }
    }

    if (RemovalType != PipQueryRemove && RemovalType != PipEject)
    {
        if (RemovalType == PipSurpriseRemove || RemovalType == PipRemoveFailed)
        {
            IopDeleteLockedDeviceNodes(DeviceObject, RelationsList, PipSurpriseRemove, FALSE, 0, NULL, NULL);
        }
    }
    else
    {
        DPRINT1("PiProcessQueryRemoveAndEject: FIXME PiNotifyUserModeDeviceRemoval()\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        Status = STATUS_NOT_IMPLEMENTED;//PiNotifyUserModeDeviceRemoval(..);
    }

    /* Tell the user-mode PnP manager that a device was removed */
    if (RemovalType == PipSurpriseRemove)
    {
        if (UnsafeRemovalEntry)
        {
            DPRINT1("PiProcessQueryRemoveAndEject: FIXME PiNotifyUserModeDeviceRemoval()\n");
            //ASSERT(FALSE); // IoDbgBreakPointEx();
            //PiNotifyUserModeDeviceRemoval(..);
// FIXME
IopQueueTargetDeviceEvent(&GUID_DEVICE_SURPRISE_REMOVAL, &DeviceNode->InstancePath);

            ExFreePool(UnsafeRemovalEntry);
        }

        DPRINT1("PiProcessQueryRemoveAndEject: FIXME PiNotifyUserModeDeviceRemoval()\n");
        //ASSERT(FALSE); // IoDbgBreakPointEx();
        //PiNotifyUserModeDeviceRemoval(..);

// FIXME
IopQueueTargetDeviceEvent(&GUID_TARGET_DEVICE_REMOVE_COMPLETE, &DeviceNode->InstancePath);
    }
    else
    {
        DPRINT1("PiProcessQueryRemoveAndEject: FIXME PiNotifyUserModeDeviceRemoval()\n");
        //ASSERT(FALSE); // IoDbgBreakPointEx();
        //PiNotifyUserModeDeviceRemoval(..);

// FIXME
IopQueueTargetDeviceEvent(&GUID_DEVICE_REMOVE_PENDING, &DeviceNode->InstancePath);
    }

    DPRINT("PiProcessQueryRemoveAndEject: REMOVE_COMPLETE - notifying kernel-mode (%X)\n", RelationsCount);

    for (ix = 0; ix < RelationsCount; ix++)
    {
        EnumDevice = RemovalDevices[ix];

        Status = IopNotifyTargetDeviceChange((PGUID)&GUID_TARGET_DEVICE_REMOVE_COMPLETE, EnumDevice, NULL, NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PiProcessQueryRemoveAndEject: Status %X\n", Status);
            ASSERT(NT_SUCCESS(Status)); // IoDbgBreakPointEx();
        }
    }

    if (RemovalType == PipRemove ||
        RemovalType == PipRemoveFailed ||
        RemovalType == PipSurpriseRemove)
    {
        IopInvalidateRelationsInList(RelationsList, RemovalType, TRUE, FALSE);
        IopRemoveIndirectRelationsFromList(RelationsList);
    }

    if (RemovalType == PipRemoveFailed ||
        RemovalType == PipSurpriseRemove)
    {
        IopUnlinkDeviceRemovalRelations(DeviceObject, RelationsList, (RemovalType != PipSurpriseRemove) + 1);
        IopQueuePendingSurpriseRemoval(DeviceObject, RelationsList, EventEntry->Argument);

        PpDevNodeUnlockTree(1);
        IopNotifyPnpWhenChainDereferenced(RemovalDevices, RelationsCount, FALSE, TRUE, NULL);

        ExFreePool(RemovalDevices);
        ExFreePool(VetoBuffer);

        return STATUS_SUCCESS;
    }

    if (DeviceNode->DockInfo.DockStatus)
    {
        DPRINT1("PiProcessQueryRemoveAndEject: FIXME IopQueryDockRemovalInterface()\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//IopQueryDockRemovalInterface(..);
    }

    IsForceDescendant = (RemovalType == PipQueryRemove || RemovalType == PipEject);
    IopDeleteLockedDeviceNodes(DeviceObject, RelationsList, PipRemove, IsForceDescendant, EventEntry->Argument, NULL, NULL);

    EjectSupported = (((DeviceNode->CapabilityFlags >> 3) & 1) != 0);
    WarmEjectSupported = (((DeviceNode->CapabilityFlags >> 16) & 1) != 0);

    if (RemovalType != PipEject)
    {
        if (!(EventEntry->Data.Flags & 2))
        {
            DPRINT1("PiProcessQueryRemoveAndEject: FIXME\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();
        }

        DPRINT1("PiProcessQueryRemoveAndEject: FIXME IopUnlinkDeviceRemovalRelations()\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//IopUnlinkDeviceRemovalRelations(..);

        IopFreeRelationList(RelationsList);
        goto Finish;
    }

    if (EjectSupported || WarmEjectSupported)
    {
        DPRINT1("PiProcessQueryRemoveAndEject: FIXME\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();

        PpDevNodeUnlockTree(1);

        DPRINT1("PiProcessQueryRemoveAndEject: FIXME IopEjectDevice()\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//IopEjectDevice(..);

        ExFreePool(RemovalDevices);
        ExFreePool(VetoBuffer);

        return STATUS_PENDING;
    }

    ASSERT(!DockInterface);

    DPRINT1("PiProcessQueryRemoveAndEject: FIXME IopUnlinkDeviceRemovalRelations()\n");
    ASSERT(FALSE); // IoDbgBreakPointEx();//IopUnlinkDeviceRemovalRelations(..);

    IopFreeRelationList(RelationsList);

    if (!EventEntry->VetoName)
    {
        DPRINT1("PiProcessQueryRemoveAndEject: FIXME PpNotifyUserModeRemovalSafe()\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();//PpNotifyUserModeRemovalSafe(DeviceObject);
    }

Finish:

    if (RemovalType == PipRemove)
    {
        DPRINT1("PiProcessQueryRemoveAndEject: FIXME PiNotifyUserModeDeviceRemoval()\n");
        //ASSERT(FALSE); // IoDbgBreakPointEx();//PiNotifyUserModeDeviceRemoval(..);

/* Tell the user-mode PnP manager that a device was removed */
IopQueueTargetDeviceEvent(&GUID_TARGET_DEVICE_REMOVE_COMPLETE, &DeviceNode->InstancePath);
    }

    ExFreePool(RemovalDevices);

    if (DockInterface)
    {
        DPRINT1("PiProcessQueryRemoveAndEject: FIXME\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    ExFreePool(VetoBuffer);
    PpDevNodeUnlockTree(1);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiProcessTargetDeviceEvent(
    _In_ PPNP_DEVICE_EVENT_ENTRY* pEventEntry)
{
    PPNP_DEVICE_EVENT_ENTRY EventEntry;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("PiProcessTargetDeviceEvent: pEventEntry %p, Status %X\n", pEventEntry, Status);

    EventEntry = *pEventEntry;

    if (PiCompareGuid(&EventEntry->Data.EventGuid, &GUID_DEVICE_QUERY_AND_REMOVE))
    {
        DPRINT("PiProcessTargetDeviceEvent: GUID_DEVICE_QUERY_AND_REMOVE\n");
        Status = PiProcessQueryRemoveAndEject(pEventEntry);
        return Status;
    }

    if (PiCompareGuid(&EventEntry->Data.EventGuid, &GUID_DEVICE_EJECT))
    {
        DPRINT("PiProcessTargetDeviceEvent: GUID_DEVICE_EJECT\n");
        Status = PiProcessQueryRemoveAndEject(pEventEntry);
        return Status;
    }

    if (PiCompareGuid(&EventEntry->Data.EventGuid, &GUID_DEVICE_ARRIVAL))
    {
        DPRINT("PiProcessTargetDeviceEvent: GUID_DEVICE_ARRIVAL\n");
        DPRINT("PiProcessTargetDeviceEvent: FIXME PiNotifyUserMode()\n");
        //PiNotifyUserMode(EventEntry);
        return Status;
    }

    if (PiCompareGuid(&EventEntry->Data.EventGuid, &GUID_DEVICE_NOOP))
    {
        DPRINT("PiProcessTargetDeviceEvent: GUID_DEVICE_NOOP\n");
        return Status;
    }

    if (PiCompareGuid(&EventEntry->Data.EventGuid, &GUID_DEVICE_SAFE_REMOVAL))
    {
        DPRINT("PiProcessTargetDeviceEvent: GUID_DEVICE_SAFE_REMOVAL\n");
        DPRINT("PiProcessTargetDeviceEvent: FIXME PiNotifyUserMode()\n");
        //PiNotifyUserMode(EventEntry);
    }

    return Status;
}

VOID
NTAPI
PiWalkDeviceList(
    _In_ PVOID Context)
{
    PPNP_DEVICE_EVENT_ENTRY EventEntry;
    UNICODE_STRING SymbolicLinkName;
    PWORK_QUEUE_ITEM WorkItem = Context;
    NTSTATUS Status;

    DPRINT("PiWalkDeviceList: WorkItem %p\n", WorkItem);
    PAGED_CODE();

    Status = KeWaitForSingleObject(&PpDeviceEventList->EventQueueMutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PiWalkDeviceList: Status %X\n", Status);
        KeAcquireGuardedMutex(&PiNotificationInProgressLock);
        KeSetEvent(&PiEventQueueEmpty, IO_NO_INCREMENT, FALSE);
        PiNotificationInProgress = FALSE;
        KeReleaseGuardedMutex(&PiNotificationInProgressLock);
        return;
    }

    while (TRUE)
    {
        KeAcquireGuardedMutex(&PpDeviceEventList->Lock);

        if (IsListEmpty(&PpDeviceEventList->List))
        {
            DPRINT("PiWalkDeviceList: IsListEmpty(&PpDeviceEventList->List)\n");
            break;
        }

        EventEntry = CONTAINING_RECORD(RemoveHeadList(&PpDeviceEventList->List),
                                       PNP_DEVICE_EVENT_ENTRY,
                                       ListEntry);

        DPRINT("PiWalkDeviceList: EventEntry %p, EventCategory %X\n", EventEntry, EventEntry->Data.EventCategory);

        KeReleaseGuardedMutex(&PpDeviceEventList->Lock);

        if (EventEntry->Data.DeviceObject)
        {
            PDEVICE_NODE DeviceNode;

            DeviceNode = IopGetDeviceNode(EventEntry->Data.DeviceObject);
            if (!DeviceNode)
            {
                DPRINT1("PiWalkDeviceList: continue. STATUS_NO_SUCH_DEVICE\n");
                PpCompleteDeviceEvent(EventEntry, STATUS_NO_SUCH_DEVICE);
                IopProcessDeferredRegistrations();
                continue;
            }
        }

        switch (EventEntry->Data.EventCategory)
        {
            case DeviceClassChangeEvent:
                DPRINT("PiWalkDeviceList: DeviceClassChangeEvent - kernel notifying\n");

                RtlInitUnicodeString(&SymbolicLinkName, EventEntry->Data.DeviceClass.SymbolicLinkName);

                IopNotifyDeviceClassChange(&EventEntry->Data,
                                           &EventEntry->Data.DeviceClass.ClassGuid,
                                           &SymbolicLinkName);

                DPRINT("PiWalkDeviceList: FIXME PiNotifyUserMode()\n");
                //PiNotifyUserMode(EventEntry);
                Status = STATUS_SUCCESS;
                break;

            case CustomDeviceEvent:
                DPRINT1("PiWalkDeviceList: FIXME CustomDeviceEvent()\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();
                Status = STATUS_NOT_IMPLEMENTED;
                break;

            case TargetDeviceChangeEvent:
                Status = PiProcessTargetDeviceEvent(&EventEntry);
                break;

            case DeviceInstallEvent:
                DPRINT1("PiWalkDeviceList: FIXME DeviceInstallEvent - user-mode notifying\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();
                //PiNotifyUserMode(EventEntry);
                Status = STATUS_SUCCESS;
                break;

            case HardwareProfileChangeEvent:
                DPRINT1("PiWalkDeviceList: FIXME HardwareProfileChangeEvent\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();
                //Status = PiNotifyUserMode(EventEntry);
                break;

            case PowerEvent:
                DPRINT1("PiWalkDeviceList: FIXME PowerEvent\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();
                //Status = PiNotifyUserMode(EventEntry);
                break;

            case VetoEvent:
                DPRINT1("PiWalkDeviceList: FIXME VetoEvent\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();
                //Status = PiNotifyUserMode(EventEntry);
                break;

            case BlockedDriverEvent:
                DPRINT1("PiWalkDeviceList: FIXME BlockedDriverEvent\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();
                //Status = PiNotifyUserMode(EventEntry);
                break;

            case InvalidIDEvent:
                DPRINT1("PiWalkDeviceList: FIXME InvalidIDEvent\n");
                ASSERT(FALSE); // IoDbgBreakPointEx();
                //Status = PiNotifyUserMode(EventEntry);
                break;

            default:
                DPRINT1("PiWalkDeviceList: Unknown EventCategory %X\n", EventEntry->Data.EventCategory);
                ASSERT(FALSE); // IoDbgBreakPointEx();
                Status = STATUS_UNSUCCESSFUL;
                break;
        }

        if (Status != STATUS_PENDING)
        {
            PpCompleteDeviceEvent(EventEntry, Status);
        }
        else
        {
            DPRINT1("PiWalkDeviceList: Status %X\n", Status);
            ASSERT(FALSE);
        }

        IopProcessDeferredRegistrations();
    }

    KeAcquireGuardedMutex(&PiNotificationInProgressLock);
    KeSetEvent(&PiEventQueueEmpty, IO_NO_INCREMENT, FALSE);
    PiNotificationInProgress = FALSE;
    IopProcessDeferredRegistrations();
    KeReleaseGuardedMutex(&PiNotificationInProgressLock);

    KeReleaseGuardedMutex(&PpDeviceEventList->Lock);

    if (WorkItem)
        ExFreePoolWithTag(WorkItem, 'IWpP');

    KeReleaseMutex(&PpDeviceEventList->EventQueueMutex, FALSE);

    DPRINT("PiWalkDeviceList: exit\n");
}

NTSTATUS
NTAPI
PiInsertEventInQueue(
    _In_ PPNP_DEVICE_EVENT_ENTRY EventEntry)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWORK_QUEUE_ITEM WorkItem = NULL;

    DPRINT("PiInsertEventInQueue: EventEntry %p, EventCategory %X\n", EventEntry, EventEntry->Data.EventCategory);
    PAGED_CODE();

    KeAcquireGuardedMutex(&PpDeviceEventList->Lock);
    KeAcquireGuardedMutex(&PiNotificationInProgressLock);

    if (PiNotificationInProgress)
    {
        DPRINT("PiInsertEventInQueue: PiNotificationInProgress TRUE\n");
    }
    else
    {
        WorkItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(*WorkItem), 'IWpP');
        if (WorkItem)
        {
            PiNotificationInProgress = TRUE;
            KeClearEvent(&PiEventQueueEmpty);
        }
        else
        {
            DPRINT1("PiInsertEventInQueue: STATUS_INSUFFICIENT_RESOURCES\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    InsertTailList(&PpDeviceEventList->List, &EventEntry->ListEntry);

    KeReleaseGuardedMutex(&PiNotificationInProgressLock);
    KeReleaseGuardedMutex(&PpDeviceEventList->Lock);

    if (!WorkItem)
        return Status;

    WorkItem->WorkerRoutine = PiWalkDeviceList;
    WorkItem->Parameter = WorkItem;
    WorkItem->List.Flink = NULL;

    ExQueueWorkItem(WorkItem, DelayedWorkQueue);

    DPRINT("PiInsertEventInQueue: queue WorkItem %X\n", WorkItem);
    return Status;
}

NTSTATUS
NTAPI
PpSynchronizeDeviceEventQueue(VOID)
{
    PPNP_DEVICE_EVENT_ENTRY EventEntry;
    NTSTATUS Status;
    KEVENT Event;
    NTSTATUS RetStatus;

    PAGED_CODE();
    DPRINT("PpSynchronizeDeviceEventQueue()\n");

    EventEntry = ExAllocatePoolWithTag(PagedPool, sizeof(*EventEntry), 'EEpP');
    if (!EventEntry)
    {
        DPRINT1("PpSynchronizeDeviceEventQueue: return STATUS_NO_MEMORY\n");
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(EventEntry, sizeof(*EventEntry));

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    EventEntry->CallerEvent = &Event;

    RtlCopyMemory(&EventEntry->Data.EventGuid, &GUID_DEVICE_NOOP, sizeof(GUID));

    EventEntry->Data.EventCategory = 1;
    EventEntry->Data.Result = (PULONG)&RetStatus;
    EventEntry->Data.TotalSize = sizeof(EventEntry->Data);

    Status = PiInsertEventInQueue(EventEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PpSynchronizeDeviceEventQueue: return STATUS_NO_MEMORY\n");
        return Status;
    }

    return KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
}

NTSTATUS
NTAPI
PpSetDeviceClassChange(
    _In_ CONST GUID* EventGuid,
    _In_ GUID* ClassGuid,
    _In_ PUNICODE_STRING SymbolicLinkName)
{
    PPNP_DEVICE_EVENT_ENTRY EventEntry;
    ULONG EventEntrySize;
    ULONG DataTotalSize;
    ULONG Length;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpSetDeviceClassChange: SymbolicLink '%wZ'\n", SymbolicLinkName);

    if (PpPnpShuttingDown)
    {
        ASSERT(FALSE);
        return STATUS_TOO_LATE;
    }

    //_SEH2_TRY

    ASSERT(EventGuid != NULL);
    ASSERT(ClassGuid != NULL);
    ASSERT(SymbolicLinkName != NULL);

    Length = SymbolicLinkName->Length;
    DataTotalSize = Length + sizeof(PLUGPLAY_EVENT_BLOCK);
    EventEntrySize = Length + sizeof(PNP_DEVICE_EVENT_ENTRY);

    EventEntry = ExAllocatePoolWithTag(PagedPool, EventEntrySize, 'EEpP');
    if (!EventEntry)
    {
        DPRINT1("PpSetDeviceClassChange: return STATUS_NO_MEMORY\n");
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(EventEntry, EventEntrySize);

    EventEntry->Data.EventCategory = DeviceClassChangeEvent;
    EventEntry->Data.TotalSize = DataTotalSize;

    RtlCopyMemory(&EventEntry->Data.EventGuid, EventGuid, sizeof(GUID));
    RtlCopyMemory(&EventEntry->Data.DeviceClass.ClassGuid, ClassGuid, sizeof(GUID));
    RtlCopyMemory(EventEntry->Data.DeviceClass.SymbolicLinkName, SymbolicLinkName->Buffer, Length);

    EventEntry->Data.DeviceClass.SymbolicLinkName[Length / sizeof(WCHAR)] = UNICODE_NULL;

    Status = PiInsertEventInQueue(EventEntry);

    //_SEH2_END;

    return Status;
}

NTSTATUS
NTAPI
PpSetTargetDeviceRemove(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN IsRemove,
    _In_ BOOLEAN IsNoRestart,
    _In_ BOOLEAN RemoveNoRestart,
    _In_ BOOLEAN IsEjectRequest,
    _In_ ULONG Problem,
    _In_ PKEVENT SyncEvent,
    _Out_ NTSTATUS* OutResult,
    _In_ PPNP_VETO_TYPE VetoType,
    _In_ PUNICODE_STRING VetoName)
{
    PPNP_DEVICE_EVENT_ENTRY EventEntry;
    PDEVICE_NODE DeviceNode;
    ULONG EventEntrySize;
    ULONG TotalSize;
    ULONG InstanceLength;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpSetTargetDeviceRemove: [%p] %X, %X, %X, %X, %X\n", DeviceObject, IsRemove, IsNoRestart, RemoveNoRestart, IsEjectRequest, Problem);

    ASSERT(DeviceObject != NULL);

    if (SyncEvent)
    {
        ASSERT(OutResult);
        *OutResult = STATUS_PENDING;
    }

    if (PpPnpShuttingDown)
    {
        DPRINT1("PpSetTargetDeviceRemove: return STATUS_TOO_LATE\n");
        return STATUS_TOO_LATE;
    }

    ObReferenceObject(DeviceObject);

    DeviceNode = IopGetDeviceNode(DeviceObject);
    ASSERT(DeviceNode);

    InstanceLength = DeviceNode->InstancePath.Length;
    TotalSize = (sizeof(PLUGPLAY_EVENT_BLOCK) + InstanceLength + sizeof(WCHAR));

    EventEntrySize = (TotalSize + (sizeof(PNP_DEVICE_EVENT_ENTRY) - sizeof(PLUGPLAY_EVENT_BLOCK)));

    EventEntry = ExAllocatePoolWithTag(PagedPool, EventEntrySize, 'EEpP');
    if (!EventEntry)
    {
        DPRINT1("PpSetTargetDeviceRemove: return STATUS_INSUFFICIENT_RESOURCES\n");
        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(EventEntry, EventEntrySize);

    EventEntry->CallerEvent = SyncEvent;
    EventEntry->VetoType = VetoType;
    EventEntry->Argument = Problem;
    EventEntry->VetoName = VetoName;

    if (IsEjectRequest)
        RtlCopyMemory(&EventEntry->Data.EventGuid, &GUID_DEVICE_EJECT, sizeof(GUID));
    else
        RtlCopyMemory(&EventEntry->Data.EventGuid, &GUID_DEVICE_QUERY_AND_REMOVE, sizeof(GUID));

    EventEntry->Data.EventCategory = TargetDeviceChangeEvent;
    EventEntry->Data.Result = (PVOID)OutResult;

    if (IsNoRestart)
        EventEntry->Data.Flags |= 2;

    if (IsRemove)
        EventEntry->Data.Flags |= 4;

    if (RemoveNoRestart)
    {
        ASSERT(IsNoRestart == FALSE);
        EventEntry->Data.Flags |= 8;
    }

    EventEntry->Data.TotalSize = TotalSize;
    EventEntry->Data.DeviceObject = DeviceObject;

    if (InstanceLength)
    {
        RtlCopyMemory(&EventEntry->Data.TargetDevice.DeviceIds,
                      DeviceNode->InstancePath.Buffer,
                      InstanceLength);
    }

    EventEntry->Data.TargetDevice.DeviceIds[InstanceLength / sizeof(WCHAR)] = UNICODE_NULL;

    Status = PiInsertEventInQueue(EventEntry);

    DPRINT("PpSetTargetDeviceRemove: return %X\n", Status);
    return Status;
}

/* EOF */

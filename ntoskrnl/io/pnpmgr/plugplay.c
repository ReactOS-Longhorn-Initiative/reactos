/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/plugplay.c
 * PURPOSE:         Plug-and-play interface routines
 * PROGRAMMERS:     Eric Kohl <eric.kohl@t-online.de>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
  #pragma alloc_text(INIT, IopInitPlugPlayEvents)
#endif

/* DATA **********************************************************************/

typedef struct _PNP_EVENT_ENTRY
{
    LIST_ENTRY ListEntry;
    PLUGPLAY_EVENT_BLOCK Event;
} PNP_EVENT_ENTRY, *PPNP_EVENT_ENTRY;

PNP_CONTROL_HANDLER PlugPlayHandlerTable[] =
{
  { 0,  12, &PiControlEnumerateDevice },
  { 1,  12, &PiControlRegisterNewDevice },
  { 2,  12, &PiControlDeregisterDevice },
  { 3,  12, &PiControlInitializeDevice },
  { 4,  12, &PiControlStartDevice },
  { 5,  12, NULL },
  { 6,  24, &PiControlQueryAndRemoveDevice },
  { 7,  16, &PiControlUserResponse },
  { 8,  16, &PiControlGenerateLegacyDevice },
  { 9,  24, &PiControlGetInterfaceDeviceList },
  { 10, 20, &PiControlGetPropertyData },
  { 11, 32, &PiControlDeviceClassAssociation },
  { 12, 20, &PiControlGetRelatedDevice },
  { 13, 20, &PiControlGetInterfaceDeviceAlias },
  { 14, 20, &PiControlGetSetDeviceStatus },
  { 15, 12, &PiControlGetDeviceDepth },
  { 16, 20, &PiControlQueryDeviceRelations },
  { 17, 16, &PiControlQueryTargetDeviceRelation },
  { 18, 32, &PiControlQueryConflictList },
  { 19, 8,  &PiControlRetrieveDockData },
  { 20, 12, &PiControlResetDevice },
  { 21, 12, &PiControlHaltDevice },
  { 22, 12, &PiControlGetBlockedDriverData },
  { 23, 0,  NULL }
};

/* GLOBALS *******************************************************************/

static LIST_ENTRY IopPnpEventQueueHead;
static KEVENT IopPnpNotifyEvent;

extern ERESOURCE PpRegistryDeviceResource;

/* FUNCTIONS *****************************************************************/

NTSTATUS CODE_SEG("INIT")
IopInitPlugPlayEvents(VOID)
{
    InitializeListHead(&IopPnpEventQueueHead);

    KeInitializeEvent(&IopPnpNotifyEvent,
                      SynchronizationEvent,
                      FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
IopQueueTargetDeviceEvent(const GUID *Guid,
                          PUNICODE_STRING DeviceIds)
{
    PPNP_EVENT_ENTRY EventEntry;
    UNICODE_STRING Copy;
    ULONG TotalSize;
    NTSTATUS Status;

    ASSERT(DeviceIds);

    /* Allocate a big enough buffer */
    Copy.Length = 0;
    Copy.MaximumLength = DeviceIds->Length + sizeof(UNICODE_NULL);
    TotalSize =
        FIELD_OFFSET(PLUGPLAY_EVENT_BLOCK, TargetDevice.DeviceIds) +
        Copy.MaximumLength;

    EventEntry = ExAllocatePool(NonPagedPool,
                                TotalSize + FIELD_OFFSET(PNP_EVENT_ENTRY, Event));
    if (!EventEntry)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Fill the buffer with the event GUID */
    RtlCopyMemory(&EventEntry->Event.EventGuid,
                  Guid,
                  sizeof(GUID));
    EventEntry->Event.EventCategory = TargetDeviceChangeEvent;
    EventEntry->Event.TotalSize = TotalSize;

    /* Fill the device id */
    Copy.Buffer = EventEntry->Event.TargetDevice.DeviceIds;
    Status = RtlAppendUnicodeStringToString(&Copy, DeviceIds);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(EventEntry);
        return Status;
    }

    InsertHeadList(&IopPnpEventQueueHead,
                   &EventEntry->ListEntry);
    KeSetEvent(&IopPnpNotifyEvent,
               0,
               FALSE);

    return STATUS_SUCCESS;
}

/* Remove the current PnP event from the tail of the event queue
   and signal IopPnpNotifyEvent if there is yet another event in the queue.
*/
NTSTATUS
IopRemovePlugPlayEvent(VOID)
{
    DPRINT("IopRemovePlugPlayEvent()\n");

    /* Remove a pnp event entry from the tail of the queue */
    if (!IsListEmpty(&IopPnpEventQueueHead))
        ExFreePool(CONTAINING_RECORD(RemoveTailList(&IopPnpEventQueueHead), PNP_EVENT_ENTRY, ListEntry));

    /* Signal the next pnp event in the queue */
    if (!IsListEmpty(&IopPnpEventQueueHead))
        KeSetEvent(&IopPnpNotifyEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_SUCCESS;
}

static PDEVICE_OBJECT
IopTraverseDeviceNode(PDEVICE_NODE Node, PUNICODE_STRING DeviceInstance)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE ChildNode;

    if (RtlEqualUnicodeString(&Node->InstancePath,
                              DeviceInstance, TRUE))
    {
        ObReferenceObject(Node->PhysicalDeviceObject);
        return Node->PhysicalDeviceObject;
    }

    /* Traversal of all children nodes */
    for (ChildNode = Node->Child;
         ChildNode != NULL;
         ChildNode = ChildNode->Sibling)
    {
        DeviceObject = IopTraverseDeviceNode(ChildNode, DeviceInstance);
        if (DeviceObject != NULL)
        {
            return DeviceObject;
        }
    }

    return NULL;
}


PDEVICE_OBJECT
IopGetDeviceObjectFromDeviceInstance(PUNICODE_STRING DeviceInstance)
{
    if (IopRootDeviceNode == NULL)
        return NULL;

    if (DeviceInstance == NULL ||
        DeviceInstance->Length == 0)
    {
        if (IopRootDeviceNode->PhysicalDeviceObject)
        {
            ObReferenceObject(IopRootDeviceNode->PhysicalDeviceObject);
            return IopRootDeviceNode->PhysicalDeviceObject;
        }
        else
            return NULL;
    }

    return IopTraverseDeviceNode(IopRootDeviceNode, DeviceInstance);
}

NTSTATUS
NTAPI
PiControlCopyUserModeCallersBuffer(
    _Out_ PVOID OutBuffer,
    _In_ PVOID Buffer,
    _In_ SIZE_T Length,
    _In_ ULONG Alignment,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN IsReadFromBuffer)
{
    ULONG_PTR buffer;
    ULONG_PTR bufferEnd;

    PAGED_CODE();

    if (AccessMode == KernelMode)
    {
        RtlCopyMemory(OutBuffer, Buffer, Length);
        return STATUS_SUCCESS;
    }

    /* UserMode */

    if (!IsReadFromBuffer)
    {
        ProbeForWrite(OutBuffer, Length, Alignment);
        goto Exit;
    }

    ASSERT((Alignment == 1) || (Alignment == 2) || (Alignment == 4) ||
           (Alignment == 8) || (Alignment == 16));

    if (!Length)
        goto Exit;

    buffer = (ULONG_PTR)Buffer;
    bufferEnd = (buffer + Length);

    if (buffer & (Alignment - 1))
        ExRaiseDatatypeMisalignment();

    if ((bufferEnd <= MmUserProbeAddress) && (bufferEnd >= buffer))
        goto Exit;

    DPRINT1("PiControlCopyUserModeCallersBuffer: buffer %p, bufferEnd %p\n", buffer, bufferEnd);
    ASSERT(FALSE); // IoDbgBreakPointEx();

Exit:
    RtlCopyMemory(OutBuffer, Buffer, Length);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiControlMakeUserModeCallersCopy(
    _Out_ PVOID* pOutBuffer,
    _In_ PVOID Buffer,
    _In_ SIZE_T Size,
    _In_ ULONG Alignment,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN IsReadFromBuffer)
{
    NTSTATUS Status;
    PVOID NewBuffer;

    PAGED_CODE();

    if (AccessMode == KernelMode)
    {
        *pOutBuffer = Buffer;
        return STATUS_SUCCESS;
    }

    if (!Size)
    {
        *pOutBuffer = NULL;
        return STATUS_SUCCESS;
    }

    if (IsReadFromBuffer)
    {
        NewBuffer = ExAllocatePoolWithQuotaTag((PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE), Size, '  pP');
        if (!NewBuffer)
        {
            *pOutBuffer = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        *pOutBuffer = NewBuffer;
    }
    else if (*pOutBuffer == NULL)
    {
        return STATUS_SUCCESS;
    }

    Status = PiControlCopyUserModeCallersBuffer(*pOutBuffer, Buffer, Size, Alignment, AccessMode, IsReadFromBuffer);
    if (NT_SUCCESS(Status))
        return Status;

    if (IsReadFromBuffer)
    {
        ExFreePoolWithTag(*pOutBuffer, '  pP');
        *pOutBuffer = NULL;
    }

    return Status;
}

/* CONTROL FUNCTIONS *********************************************************/

NTSTATUS NTAPI PiControlEnumerateDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlRegisterNewDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlDeregisterDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlInitializeDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlStartDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlQueryAndRemoveDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlUserResponse(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    DPRINT("PiControlUserResponse: Class %X, Data %p, Length %X\n", PnPControlClass, PnPControlData, PnPControlDataLength);

    PAGED_CODE();
    ASSERT(PnPControlClass == 7); // PlugPlayControlUserResponse

    if (!PnPControlData || PnPControlDataLength != sizeof(PLUGPLAY_CONTROL_USER_RESPONSE_DATA))
    {
        DPRINT1("PiControlUserResponse: FIXME NtPlugPlayControl(PlugPlayControlUserResponse, NULL, 0) in usetup/devinst.c\n"); 
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    return IopRemovePlugPlayEvent();
}

NTSTATUS NTAPI PiControlGenerateLegacyDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlGetInterfaceDeviceList(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlGetPropertyData(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlDeviceClassAssociation(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PiGetRelatedDevice(
     _In_ PUNICODE_STRING DeviceInstance,
     _Out_ PWSTR OutInstancePath,
     _Out_ PUSHORT OutInstanceSize,
     _In_ ULONG Relationship)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE DeviceNode;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("PiGetRelatedDevice: Instance '%wZ', Relationship %X\n", DeviceInstance, Relationship);

    PpDevNodeLockTree(0);

    DeviceObject = IopDeviceObjectFromDeviceInstance(DeviceInstance);
    if (!DeviceObject)
    {
        DPRINT("PiGetRelatedDevice: Status = STATUS_NO_SUCH_DEVICE\n");
        Status = STATUS_NO_SUCH_DEVICE;
        goto Exit;
    }

    DeviceNode = IopGetDeviceNode(DeviceObject);
    if (!DeviceNode)
    {
        DPRINT("PiGetRelatedDevice: Status = STATUS_NO_SUCH_DEVICE\n");
        Status = STATUS_NO_SUCH_DEVICE;
        goto Exit;
    }

    DPRINT("PiGetRelatedDevice: Device %p, Node %p\n", DeviceObject, DeviceNode);

    if (DeviceNode->State == DeviceNodeDeleted ||
        DeviceNode->State == DeviceNodeDeletePendingCloses)
    {
        DPRINT("PiGetRelatedDevice: Status = STATUS_NO_SUCH_DEVICE\n");
        Status = STATUS_NO_SUCH_DEVICE;
        goto Exit;
    }

    switch (Relationship)
    {
        case 1: // Parent
        {
            DeviceNode = DeviceNode->Parent;
            DPRINT1("PiGetRelatedDevice: Parent Node %X\n", DeviceNode);
            break;
        }
        case 2: // Child
        {
            DeviceNode = DeviceNode->Child;
            DPRINT1("PiGetRelatedDevice: Child Node %X\n", DeviceNode);
            goto NextNode;
        }
        case 3: // Sibling
        {
            DeviceNode = DeviceNode->Sibling;
            DPRINT1("PiGetRelatedDevice: Sibling Node %X\n", DeviceNode);
NextNode:
            while (DeviceNode &&
                   DeviceNode->Flags & DNF_HAS_PROBLEM &&
                   DeviceNode->Problem == CM_PROB_DEVICE_NOT_THERE &&
                   DeviceNode->Flags & DNF_LEGACY_DRIVER)
            {
                DeviceNode = DeviceNode->Sibling;
            }

            KeEnterCriticalRegion();
            ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

            while (DeviceNode)
            {
                if (DeviceNode->InstancePath.Length)
                {
                    deviceObject = IopDeviceObjectFromDeviceInstance(&DeviceNode->InstancePath);
                    if (deviceObject)
                    {
                        ObDereferenceObject(deviceObject);
                        DPRINT("PiGetRelatedDevice: device %X Node %X\n", deviceObject, DeviceNode);
                        break;
                    }
                }

                DeviceNode = DeviceNode->Sibling;
            }

            ExReleaseResourceLite(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();

            break;
        }
        default:
        {
            DPRINT("PiGetRelatedDevice: STATUS_INVALID_PARAMETER\n");
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    if (!DeviceNode)
    {
        DPRINT("PiGetRelatedDevice: STATUS_NO_SUCH_DEVICE\n");
        Status = STATUS_NO_SUCH_DEVICE;
        goto Exit;
    }

    if (*OutInstanceSize <= DeviceNode->InstancePath.Length)
    {
        DPRINT1("PiGetRelatedDevice: STATUS_BUFFER_TOO_SMALL\n");
        Status = STATUS_BUFFER_TOO_SMALL;
        *OutInstanceSize = (DeviceNode->InstancePath.Length + sizeof(WCHAR));
    }
    else
    {
        RtlCopyMemory(OutInstancePath, DeviceNode->InstancePath.Buffer, DeviceNode->InstancePath.Length);
        DPRINT1("PiGetRelatedDevice: '%S', %X\n", DeviceNode->InstancePath.Buffer, DeviceNode->InstancePath.Length);

        OutInstancePath[DeviceNode->InstancePath.Length / sizeof(WCHAR)] = 0;
        *OutInstanceSize = DeviceNode->InstancePath.Length;
    }

Exit:
    PpDevNodeUnlockTree(0);

    if (DeviceObject)
        ObDereferenceObject(DeviceObject);

    return Status;
}

NTSTATUS NTAPI PiControlGetRelatedDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA RelatedDeviceData = PnPControlData;
    UNICODE_STRING InstanceName;
    PWSTR Instance;
    USHORT Size;
    NTSTATUS status;
    NTSTATUS Status;
  
    DPRINT("PiControlGetRelatedDevice: Class %X, Data %p, Length %X\n", PnPControlClass, PnPControlData, PnPControlDataLength);
    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlGetRelatedDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA));

    InstanceName.Length = RelatedDeviceData->TargetDeviceInstance.Length;
    InstanceName.MaximumLength = InstanceName.Length;

    if (!InstanceName.Length)
    {
        DPRINT1("PiControlResetDevice: STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (InstanceName.Length > 0x190) // ?
    {
        DPRINT1("PiControlResetDevice: STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (InstanceName.Length & 1)
    {
        DPRINT1("PiControlResetDevice: STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (RelatedDeviceData->RelatedDeviceInstance &&
        RelatedDeviceData->RelatedDeviceInstanceLength)
    {
        Size = (RelatedDeviceData->RelatedDeviceInstanceLength * sizeof(WCHAR));

        if (AccessMode == KernelMode)
        {
            Instance = RelatedDeviceData->RelatedDeviceInstance;
        }
        else
        {
            Instance = ExAllocatePoolWithQuotaTag((PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE), Size, '  pP');
            if (!Instance)
            {
                DPRINT1("PiControlResetDevice: STATUS_INSUFFICIENT_RESOURCES\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(Instance, Size);
        }
    }
    else
    {
        Instance = NULL;
        Size = 0;
    }

    InstanceName.Buffer = NULL;

    Status = PiControlMakeUserModeCallersCopy((PVOID *)&InstanceName.Buffer,
                                              RelatedDeviceData->TargetDeviceInstance.Buffer,
                                              InstanceName.Length,
                                              2,
                                              AccessMode,
                                              TRUE);//Read
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PiControlGetRelatedDevice: Status %X\n", Status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        goto Exit;
    }

    Status = PiGetRelatedDevice(&InstanceName, Instance, &Size, RelatedDeviceData->Relation);
    if (!Instance)
    {
        RelatedDeviceData->RelatedDeviceInstanceLength = (Size / sizeof(WCHAR));
        goto Exit;
    }

    status = PiControlMakeUserModeCallersCopy((PVOID *)&RelatedDeviceData->RelatedDeviceInstance,
                                              Instance,
                                              (RelatedDeviceData->RelatedDeviceInstanceLength * sizeof(WCHAR)),
                                              2,
                                              AccessMode,
                                              FALSE);//Wtite
    if (!NT_SUCCESS(status))
    {
        DPRINT("PiControlGetRelatedDevice: status %X\n", status);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        Status = status;
    }

    RelatedDeviceData->RelatedDeviceInstanceLength = (Size / sizeof(WCHAR));

Exit:

    if (AccessMode != KernelMode)
    {
        if (InstanceName.Buffer)
            ExFreePool(InstanceName.Buffer);

        if (Instance)
            ExFreePoolWithTag(Instance, '  pP');
    }

    return Status;
}

NTSTATUS NTAPI PiControlGetInterfaceDeviceAlias(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
PiControlGetUserFlagsFromDeviceNode(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PULONG OutDeviceStatus)
{
    ULONG DeviceStatus;

    PAGED_CODE();

    DeviceStatus = 0x01800000;

    if (PipAreDriversLoaded(DeviceNode))
        DeviceStatus = 0x01800002;

    if (PipIsDevNodeDNStarted(DeviceNode))
        DeviceStatus |= 0x00000008;

    /* UserFlags */

    if (DeviceNode->UserFlags & 0x0001)
        DeviceStatus |= 0x00040000;

    if (DeviceNode->UserFlags & 0x0002)
        DeviceStatus |= 0x40000000;

    if (DeviceNode->UserFlags & 0x0004)
        DeviceStatus |= 0x00000100;

    /* Flags */

    if (DeviceNode->Flags & 0x000004000)
        DeviceStatus |= 0x00008000;

    if (DeviceNode->Flags & 0x00002000)
        DeviceStatus |= 0x00000400;

    if (DeviceNode->Flags & 0x00100000)
        DeviceStatus |= 0x00000040;

    if (DeviceNode->Flags & 0x00001000)
        DeviceStatus |= 0x00001000;

    if (DeviceNode->Flags & 0x00200000)
        DeviceStatus |= 0x00000200;

    if (!DeviceNode->DisableableDepends)
        DeviceStatus |= 0x00002000;

    *OutDeviceStatus = DeviceStatus;
}

NTSTATUS NTAPI PiControlGetSetDeviceStatus(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    PPLUGPLAY_CONTROL_STATUS_DATA StatusData;
    PDEVICE_NODE DeviceNode = NULL;
    UNICODE_STRING DeviceInstance;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT Event;
    KEVENT event;
    ULONG_PTR RequestArgument;
    ULONG RequestType;
    UCHAR ReorderingBarrier;
    NTSTATUS* OutStatus;
    NTSTATUS status;
    NTSTATUS Status;

    DPRINT("PiControlGetSetDeviceStatus : Class %X Data %p, Length %X\n", PnPControlClass, PnPControlData, PnPControlDataLength);
    PAGED_CODE();

    ASSERT(PnPControlClass == 14); // PlugPlayControlDeviceStatus
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_STATUS_DATA));

    StatusData = PnPControlData;

    DeviceInstance.MaximumLength = StatusData->DeviceInstance.Length;
    DeviceInstance.Length = StatusData->DeviceInstance.Length;

    if (!StatusData->DeviceInstance.Length)
    {
        DPRINT1("PiControlGetSetDeviceStatus : STATUS_INVALID_PARAMETER\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (StatusData->DeviceInstance.Length > 0x190) // ?
    {
        DPRINT1("PiControlGetSetDeviceStatus : STATUS_INVALID_PARAMETER\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (StatusData->DeviceInstance.Length & 1)
    {
        DPRINT1("PiControlGetSetDeviceStatus : STATUS_INVALID_PARAMETER\n");
        return STATUS_INVALID_PARAMETER;
    }

    Status = PiControlMakeUserModeCallersCopy((PVOID *)&DeviceInstance.Buffer,
                                              StatusData->DeviceInstance.Buffer,
                                              StatusData->DeviceInstance.Length,
                                              2,
                                              AccessMode,
                                              TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PiControlGetSetDeviceStatus : Status %X\n", Status);
        return Status;
    }

    PpDevNodeLockTree(0);

    DeviceObject = IopDeviceObjectFromDeviceInstance(&DeviceInstance);

    if (AccessMode && DeviceInstance.Buffer)
        ExFreePool(DeviceInstance.Buffer);

    if (DeviceObject)
        DeviceNode = IopGetDeviceNode(DeviceObject);

    PpDevNodeUnlockTree(0);

    if (!DeviceNode || DeviceNode == IopRootDeviceNode)
    {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (StatusData->Operation == 0)
    {
        PiControlGetUserFlagsFromDeviceNode(DeviceNode, &StatusData->DeviceStatus);
        StatusData->DeviceProblem = DeviceNode->Problem;
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (StatusData->Operation == 1)
    {
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        OutStatus = &status;
        Event = &event;

        ReorderingBarrier = 0;
        RequestArgument = (ULONG_PTR)StatusData;
        RequestType = PipEnumSetProblem;

        DPRINT1("PiControlGetSetDeviceStatus : StatusData->Operation == 1\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();

        Status = PipRequestDeviceAction(DeviceObject, RequestType, ReorderingBarrier, RequestArgument, Event, OutStatus);
        if (NT_SUCCESS(Status))
        {
            Status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            if (Status != STATUS_SUCCESS)
                Status = status;
        }

        goto Exit;
    }

    if (StatusData->Operation == 2)
    {
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        OutStatus = &status;
        Event = &event;

        ReorderingBarrier = 0;
        RequestArgument = 0;
        RequestType = PipEnumGetSetDeviceStatus;

        DPRINT1("PiControlGetSetDeviceStatus : StatusData->Operation == 2\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();

        Status = PipRequestDeviceAction(DeviceObject, RequestType, ReorderingBarrier, RequestArgument, Event, OutStatus);
        if (NT_SUCCESS(Status))
        {
            Status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            if (Status != STATUS_SUCCESS)
                Status = status;
        }

        goto Exit;
    }

    DPRINT1("PiControlGetSetDeviceStatus : StatusData->Operation %X\n", StatusData->Operation);
    ASSERT(FALSE); // IoDbgBreakPointEx();
    Status = STATUS_SUCCESS;

Exit:
    if (DeviceObject)
        ObDereferenceObject(DeviceObject);

    return Status;
}

NTSTATUS NTAPI PiControlGetDeviceDepth(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlQueryDeviceRelations(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlQueryTargetDeviceRelation(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlQueryConflictList(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlRetrieveDockData(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PiQueueDeviceRequest(
    _In_ PUNICODE_STRING DeviceInstance,
    _In_ PIP_ENUM_TYPE RequestType,
    _In_ ULONG_PTR RequestArgument,
    _In_ BOOLEAN IsWait)
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    KEVENT Event;

    PAGED_CODE();

    DeviceObject = IopDeviceObjectFromDeviceInstance(DeviceInstance);

    if (!DeviceObject)
    {
        DPRINT1("PiQueueDeviceRequest: return STATUS_NO_SUCH_DEVICE\n");
        return STATUS_NO_SUCH_DEVICE;
    }

    if (!IopGetDeviceNode(DeviceObject))
    {
        DPRINT1("PiQueueDeviceRequest: return STATUS_NO_SUCH_DEVICE\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_NO_SUCH_DEVICE;
    }

    if (IsWait)
        KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Status = PipRequestDeviceAction(DeviceObject,
                                    RequestType,
                                    0,
                                    RequestArgument,
                                    (IsWait != FALSE ? &Event : NULL),
                                    NULL);

    if (NT_SUCCESS(Status) && IsWait)
        Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    ObDereferenceObject(DeviceObject);

    return Status;
}

NTSTATUS NTAPI PiControlResetDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    PPLUGPLAY_CONTROL_RESET_DEVICE_DATA ResetData = PnPControlData;
    USHORT Size = ResetData->DeviceInstance.Length;
    UNICODE_STRING DeviceInstance;
    NTSTATUS Status;

    DPRINT("PiControlResetDevice : Class %X, Data %p, Length %X\n", PnPControlClass, PnPControlData, PnPControlDataLength);
    PAGED_CODE();

    DeviceInstance.Length = Size;
    DeviceInstance.MaximumLength = Size;

    if (!ResetData->DeviceInstance.Length)
    {
        DPRINT1("PiControlResetDevice : STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (Size > 0x190) // ?
    {
        DPRINT1("PiControlResetDevice : STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    if (Size & 1)
    {
        DPRINT1("PiControlResetDevice : STATUS_INVALID_PARAMETER\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER;
    }

    Status = PiControlMakeUserModeCallersCopy((PVOID *)&DeviceInstance.Buffer,
                                              ResetData->DeviceInstance.Buffer,
                                              Size,
                                              2,
                                              AccessMode,
                                              TRUE);//Read
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PiControlResetDevice : Status %X\n", Status);
        return Status;
    }

    Status = PiQueueDeviceRequest(&DeviceInstance, PipEnumResetDevice, 0, TRUE);

    if (AccessMode == KernelMode)
        return Status;

    if (DeviceInstance.Buffer)
        ExFreePool(DeviceInstance.Buffer);

    return Status;
}

NTSTATUS NTAPI PiControlHaltDevice(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI PiControlGetBlockedDriverData(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode)
{
    UNIMPLEMENTED;
    ASSERT(FALSE); // IoDbgBreakPointEx();
    return STATUS_NOT_IMPLEMENTED;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * Plug and Play event structure used by NtGetPlugPlayEvent.
 *
 * EventGuid
 *    Can be one of the following values:
 *       GUID_HWPROFILE_QUERY_CHANGE
 *       GUID_HWPROFILE_CHANGE_CANCELLED
 *       GUID_HWPROFILE_CHANGE_COMPLETE
 *       GUID_TARGET_DEVICE_QUERY_REMOVE
 *       GUID_TARGET_DEVICE_REMOVE_CANCELLED
 *       GUID_TARGET_DEVICE_REMOVE_COMPLETE
 *       GUID_PNP_CUSTOM_NOTIFICATION
 *       GUID_PNP_POWER_NOTIFICATION
 *       GUID_DEVICE_* (see above)
 *
 * EventCategory
 *    Type of the event that happened.
 *
 * Result
 *    ?
 *
 * Flags
 *    ?
 *
 * TotalSize
 *    Size of the event block including the device IDs and other
 *    per category specific fields.
 */

/*
 * NtGetPlugPlayEvent
 *
 * Returns one Plug & Play event from a global queue.
 *
 * Parameters
 *    Reserved1
 *    Reserved2
 *       Always set to zero.
 *
 *    Buffer
 *       The buffer that will be filled with the event information on
 *       successful return from the function.
 *
 *    BufferSize
 *       Size of the buffer pointed by the Buffer parameter. If the
 *       buffer size is not large enough to hold the whole event
 *       information, error STATUS_BUFFER_TOO_SMALL is returned and
 *       the buffer remains untouched.
 *
 * Return Values
 *    STATUS_PRIVILEGE_NOT_HELD
 *    STATUS_BUFFER_TOO_SMALL
 *    STATUS_SUCCESS
 *
 * Remarks
 *    This function isn't multi-thread safe!
 *
 * @implemented
 */
NTSTATUS
NTAPI
NtGetPlugPlayEvent(IN ULONG Reserved1,
                   IN ULONG Reserved2,
                   OUT PPLUGPLAY_EVENT_BLOCK Buffer,
                   IN ULONG BufferSize)
{
    PPNP_EVENT_ENTRY Entry;
    NTSTATUS Status;

    DPRINT("NtGetPlugPlayEvent() called\n");

    /* Function can only be called from user-mode */
    if (KeGetPreviousMode() == KernelMode)
    {
        DPRINT1("NtGetPlugPlayEvent cannot be called from kernel mode!\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Check for Tcb privilege */
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                UserMode))
    {
        DPRINT1("NtGetPlugPlayEvent: Caller does not hold the SeTcbPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Wait for a PnP event */
    DPRINT("Waiting for pnp notification event\n");
    Status = KeWaitForSingleObject(&IopPnpNotifyEvent,
                                   UserRequest,
                                   UserMode,
                                   FALSE,
                                   NULL);
    if (!NT_SUCCESS(Status) || Status == STATUS_USER_APC)
    {
        DPRINT("KeWaitForSingleObject() failed (Status %lx)\n", Status);
        ASSERT(Status == STATUS_USER_APC);
        return Status;
    }

    /* Get entry from the tail of the queue */
    Entry = CONTAINING_RECORD(IopPnpEventQueueHead.Blink,
                              PNP_EVENT_ENTRY,
                              ListEntry);

    /* Check the buffer size */
    if (BufferSize < Entry->Event.TotalSize)
    {
        DPRINT1("Buffer is too small for the pnp-event\n");
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy event data to the user buffer */
    _SEH2_TRY
    {
        ProbeForWrite(Buffer,
                      Entry->Event.TotalSize,
                      sizeof(UCHAR));
        RtlCopyMemory(Buffer,
                      &Entry->Event,
                      Entry->Event.TotalSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    DPRINT("NtGetPlugPlayEvent() done\n");

    return STATUS_SUCCESS;
}

/* A function for doing various Plug & Play operations from user mode.

    PlugPlayControlClass
       0x00   Reenumerate device tree

              Buffer points to UNICODE_STRING decribing the instance
              path (like "HTREE\ROOT\0" or "Root\ACPI_HAL\0000"). For
              more information about instance paths see !devnode command
              in kernel debugger or look at "Inside Windows 2000" book,
              chapter "Driver Loading, Initialization, and Installation".

       0x01   Register new device
       0x02   Deregister device
       0x03   Initialize device
       0x04   Start device
       0x06   Query and remove device
       0x07   User response

              Called after processing the message from NtGetPlugPlayEvent.

       0x08   Generate legacy device
       0x09   Get interface device list
       0x0A   Get property data
       0x0B   Device class association (Registration)
       0x0C   Get related device
       0x0D   Get device interface alias
       0x0E   Get/set/clear device status
       0x0F   Get device depth
       0x10   Query device relations
       0x11   Query target device relation
       0x12   Query conflict list
       0x13   Retrieve dock data
       0x14   Reset device
       0x15   Halt device
       0x16   Get blocked driver data

    Buffer
       The buffer contains information that is specific to each control
       code. The buffer is read-only.

    BufferSize
       Size of the buffer pointed by the Buffer parameter. If the
       buffer size specifies incorrect value for specified control
       code, error ??? is returned.

 Return Values
    STATUS_PRIVILEGE_NOT_HELD
    STATUS_SUCCESS
    ...

    halfplemented
*/
NTSTATUS
NTAPI
NtPlugPlayControl(
    _In_ PLUGPLAY_CONTROL_CLASS PnPControlClass,
    _Inout_ PVOID Buffer,
    _In_ ULONG BufferLength)
{
    KPROCESSOR_MODE AccessMode = KeGetCurrentThread()->PreviousMode;
    PPNP_CONTROL_HANDLER PlugPlayEntry;
    PVOID PnPControlData;
    SIZE_T PnPControlDataLength;
    NTSTATUS Status;
    NTSTATUS status;

    DPRINT("NtPlugPlayControl: PnPControlClass %X Buffer %p, BufferLength %X\n", PnPControlClass, Buffer, BufferLength);
    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        if (!SeSinglePrivilegeCheck(SeTcbPrivilege, UserMode))
        {
            DPRINT1("NtPlugPlayControl: SecurityCheck failed\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    if (PnPControlClass >= MaxPlugPlayControl) // 0x17
    {
        DPRINT1("NtPlugPlayControl: Max %X, Class %X, Length %X\n", MaxPlugPlayControl, PnPControlClass, BufferLength);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER_1;
    }

    PlugPlayEntry = &PlugPlayHandlerTable[PnPControlClass];
    if (!PlugPlayEntry)
    {
        DPRINT1("NtPlugPlayControl: Unknown control class, Class %X, Length %X\n", PnPControlClass, BufferLength);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER_1;
    }

    if (PlugPlayEntry->ControlCode != PnPControlClass)
    {
        DPRINT1("NtPlugPlayControl: Table isn't correctly (%X)!\n", PnPControlClass);
        ASSERT(PlugPlayEntry->ControlCode == PnPControlClass);
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INTERNAL_ERROR;
    }

    if (PlugPlayEntry->Function == NULL)
    {
        DPRINT1("NtPlugPlayControl: PlugPlayEntry->Function == NULL\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_NOT_IMPLEMENTED;
    }

    PnPControlDataLength = BufferLength;

    if (PlugPlayEntry->Size != BufferLength)
    {
        DPRINT("NtPlugPlayControl: FIXME! Invalid size. Class %X, Length %X\n", PnPControlClass, BufferLength);
#if 0 // FIXME umpnpmgr
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return STATUS_INVALID_PARAMETER_MIX;
#endif
    }

    Status = PiControlMakeUserModeCallersCopy(&PnPControlData,
                                              Buffer,
                                              BufferLength,
                                              sizeof(ULONG),
                                              AccessMode,
                                              TRUE); // Read
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return Status;
    }

    Status = (PlugPlayEntry->Function)(PnPControlClass, PnPControlData, PnPControlDataLength, AccessMode);

    if (NT_ERROR(Status) && (Status != STATUS_BUFFER_TOO_SMALL))
        goto Exit;

    status = PiControlMakeUserModeCallersCopy(&Buffer,
                                              PnPControlData,
                                              PnPControlDataLength,
                                              sizeof(ULONG),
                                              AccessMode,
                                              FALSE); // Write
    if (!NT_SUCCESS(status))
    {
        ASSERT(FALSE); // IoDbgBreakPointEx();
        Status = status;
    }

Exit:

    if (AccessMode && PnPControlData)
        ExFreePool(PnPControlData);

    return Status;
}

/* EOF */
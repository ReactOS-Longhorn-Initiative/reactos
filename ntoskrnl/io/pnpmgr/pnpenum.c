/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpenum.c
 * PURPOSE:         Device enumeration functions
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern PNP_ALLOCATE_RESOURCES_ROUTINE IopAllocateBootResourcesRoutine;

extern ERESOURCE PpRegistryDeviceResource;

extern KSPIN_LOCK IopPnPSpinLock;
extern LIST_ENTRY IopPnpEnumerationRequestList;
extern KEVENT PiEnumerationLock;

extern ULONG InitSafeBootMode;

extern BOOLEAN PnPBootDriversLoaded;
extern BOOLEAN PnPBootDriversInitialized;
extern BOOLEAN PiCriticalDeviceDatabaseEnabled;
extern BOOLEAN PnpSystemInit;
extern BOOLEAN PpPnpShuttingDown;
extern BOOLEAN IopBootConfigsReserved;
extern PLIST_ENTRY IopGroupTable;
extern USHORT IopGroupIndex;

/* DATA **********************************************************************/

WORK_QUEUE_ITEM PipDeviceEnumerationWorkItem;
BOOLEAN PipEnumerationInProgress;

/* See DDK "Types of WDM Drivers" */
typedef enum _PIP_DRIVER_TYPE
{
    LowerDeviceFilters,
    LowerClassFilters,
    DeviceService,
    UpperDeviceFilters,
    UpperClassFilters,
    PipMaxServiceType
} PIP_DRIVER_TYPE; 

typedef struct _DRIVER_ADD_DEVICE_ENTRY
{
    PDRIVER_OBJECT DriverObject;
    struct _DRIVER_ADD_DEVICE_ENTRY *NextEntry;
} DRIVER_ADD_DEVICE_ENTRY, *PDRIVER_ADD_DEVICE_ENTRY;

typedef struct _DRIVER_ADD_DEVICE_CONTEXT
{
    PDEVICE_NODE DeviceNode;
    BOOLEAN EnableLoadDriver;
    UCHAR Padded[3];
    SERVICE_LOAD_TYPE * DriverLoadType;
    PDRIVER_ADD_DEVICE_ENTRY DriverLists[PipMaxServiceType];
} DRIVER_ADD_DEVICE_CONTEXT, *PDRIVER_ADD_DEVICE_CONTEXT;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
PpQueryID(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BUS_QUERY_ID_TYPE IdType,
    _Out_ PWCHAR *OutID,
    _In_ PULONG OutIdSize)
{
    PUNICODE_STRING ServiceName;
    ULONG MaxSeparators;
    SIZE_T Size;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpQueryID: DeviceNode - %X, IdType - %X\n", DeviceNode, IdType);

    ASSERT(IdType == BusQueryDeviceID ||
           IdType == BusQueryInstanceID ||
           IdType == BusQueryHardwareIDs ||
           IdType == BusQueryCompatibleIDs);

    *OutIdSize = 0;

    Status = PpIrpQueryID(DeviceNode->PhysicalDeviceObject, IdType, OutID);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PpQueryID: Status - %X\n", Status);
        goto ErrorExit;
    }

    switch (IdType)
    {
        case BusQueryDeviceID:
            ServiceName = &DeviceNode->Parent->ServiceName;
            MaxSeparators = MAX_SEPARATORS_DEVICEID;

            Size = PiFixupID(*OutID,
                             MAX_DEVICE_ID_LEN,
                             FALSE,
                             MaxSeparators,
                             ServiceName);

            *OutIdSize = Size * sizeof(WCHAR);
            break;

        case BusQueryHardwareIDs:
        case BusQueryCompatibleIDs:

            Size = PiFixupID(*OutID,
                             MAX_DEVICE_ID_LEN,
                             TRUE,
                             MAX_SEPARATORS_MULTI_SZ,
                             &DeviceNode->Parent->ServiceName);

            *OutIdSize = Size * sizeof(WCHAR);
            break;

        case BusQueryInstanceID:
            ServiceName = &DeviceNode->Parent->ServiceName;
            MaxSeparators = MAX_SEPARATORS_INSTANCEID;

            Size = PiFixupID(*OutID,
                             MAX_DEVICE_ID_LEN,
                             FALSE,
                             MaxSeparators,
                             ServiceName);

            *OutIdSize = Size * sizeof(WCHAR);
            break;

        default:
            *OutIdSize = 0;
            break;
    }

    if (*OutIdSize == 0)
    {
        Status = STATUS_PNP_INVALID_ID;
    }

    if (NT_SUCCESS(Status))
    {
        return Status;
    }

ErrorExit:

    DPRINT("PpIrpQueryID: Error Status %X\n", Status);

    if (Status == STATUS_PNP_INVALID_ID || IdType == BusQueryDeviceID)
    {
        DPRINT("PpIrpQueryID: Set CM_PROB_INVALID_DATA\n");
        PipSetDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA);

        if (!(DeviceNode->Parent->Flags & DNF_CHILD_WITH_INVALID_ID))
        {
            DeviceNode->Parent->Flags |= DNF_CHILD_WITH_INVALID_ID;

            DPRINT("PpIrpQueryID: FIXME PpSetInvalidIDEvent\n");
        }
    }

    if (Status == STATUS_PNP_INVALID_ID)
    {
        DPRINT("PpIrpQueryID: Invalid ID. ServiceName - %wZ\n",
               &DeviceNode->Parent->ServiceName);

        ASSERT(Status != STATUS_PNP_INVALID_ID);
    }
    else
    {
        if (IdType || Status == STATUS_INSUFFICIENT_RESOURCES)
        {
            if (*OutID)
            {
                ExFreePool(*OutID);
                *OutID = NULL;
                *OutIdSize = 0;
            }

            return Status;
        }

        DPRINT("PpIrpQueryID: FIXME Log\n");
        DPRINT("PpIrpQueryID: ServiceName - %wZ, Status - %X\n",
               &DeviceNode->Parent->ServiceName, Status);

        ASSERT(IdType != BusQueryDeviceID);
    }

    if (*OutID)
    {
        ExFreePool(*OutID);
        *OutID = NULL;
        *OutIdSize = 0;
    }

    return Status;
}

NTSTATUS
NTAPI
PpSaveDeviceCapabilities(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PDEVICE_CAPABILITIES DeviceCapabilities)
{
    UNICODE_STRING ValueName;
    HANDLE KeyHandle = NULL;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpSaveDeviceCapabilities: DeviceNode - %p, InstancePath - %wZ\n",
           DeviceNode, &DeviceNode->InstancePath);

    ASSERT(DeviceNode);
    ASSERT(DeviceCapabilities);

    Status = PnpDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject,
                                             &KeyHandle,
                                             KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PpSaveDeviceCapabilities: Status - %X, Pdo - %p\n",
               Status, DeviceNode->PhysicalDeviceObject);

        ASSERT(NT_SUCCESS(Status));
        return Status;
    }

    ASSERT(KeyHandle);

    if (DeviceNode->Flags & DNF_HAS_BOOT_CONFIG)
    {
        DeviceCapabilities->SurpriseRemovalOK = 0;
    }

    DeviceNode->CapabilityFlags = *(PULONG)((ULONG_PTR)&DeviceCapabilities->Version +
                                            sizeof(DeviceCapabilities->Version));

    RtlInitUnicodeString(&ValueName, L"Capabilities");

    ZwSetValueKey(KeyHandle,
                  &ValueName,
                  0,
                  REG_DWORD,
                  &DeviceNode->CapabilityFlags,
                  sizeof(DeviceNode->CapabilityFlags));

    RtlInitUnicodeString(&ValueName, L"UINumber");

    if (DeviceCapabilities->UINumber == -1)
    {
        ZwDeleteValueKey(KeyHandle, &ValueName);
    }
    else
    {
        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_DWORD,
                      &DeviceCapabilities->UINumber,
                      sizeof(DeviceCapabilities->UINumber));
    }

    ZwClose(KeyHandle);
    return Status;
}

NTSTATUS
NTAPI
IopQueryAndSaveDeviceNodeCapabilities(
    _In_ PDEVICE_NODE DeviceNode)
{
    DEVICE_CAPABILITIES DeviceCapabilities;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopQueryAndSaveDeviceNodeCapabilities: DeviceNode - %p\n", DeviceNode);

    ASSERT(DeviceNode);

    Status = PpIrpQueryCapabilities(DeviceNode->PhysicalDeviceObject,
                                    &DeviceCapabilities);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopQueryAndSaveDeviceNodeCapabilities: Status - %X\n", Status);
        return Status;
    }

    Status = PpSaveDeviceCapabilities(DeviceNode, &DeviceCapabilities);

    return Status;
}

static
VOID
NTAPI
IopIncDisableableDepends(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVICE_NODE node;

    for (node = DeviceNode;
         node && InterlockedIncrement((PLONG)&node->DisableableDepends) == 1;
         node = node->Parent)
    {
        ;
    }
}

static
VOID
NTAPI
IopDecDisableableDepends(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVICE_NODE node;

    for (node = DeviceNode;
         node && !InterlockedDecrement((PLONG)&node->DisableableDepends);
         node = node->Parent)
    {
        ;
    }
}

NTSTATUS
NTAPI
IopResourceRequirementsChanged(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG RequestArgument)
{
    PAGED_CODE();
    DPRINT("IopResourceRequirementsChanged: DeviceObject %p, RequestArgument %X\n", DeviceObject, RequestArgument);

    return PipRequestDeviceAction(DeviceObject,
                                  PipEnumIoResourceChanged,
                                  0,//ReorderingBarrier
                                  RequestArgument,
                                  NULL,
                                  NULL);
}

NTSTATUS
NTAPI
PiProcessQueryDeviceState(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_NODE DeviceNode;
    PNP_DEVICE_STATE State;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PiProcessQueryDeviceState: DeviceObject - %p\n", DeviceObject);

    Status = IopQueryDeviceState(DeviceObject, &State);
    if (!NT_SUCCESS(Status))
        return STATUS_SUCCESS;

    DeviceNode = IopGetDeviceNode(DeviceObject);

    if (State & PNP_DEVICE_DONT_DISPLAY_IN_UI)
    {
        DeviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;
    }
    else
    {
        DeviceNode->UserFlags &= ~DNUF_DONT_SHOW_IN_UI;
    }

    if (State & PNP_DEVICE_NOT_DISABLEABLE)
    {
        DeviceNode->UserFlags = DeviceNode->UserFlags;

        if (!(DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE))
        {
            DeviceNode->UserFlags = (DeviceNode->UserFlags | DNUF_NOT_DISABLEABLE);
            IopIncDisableableDepends(DeviceNode);
        }
    }
    else
    {
        if (DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE)
        {
            IopDecDisableableDepends(DeviceNode);
            DeviceNode->UserFlags &= ~DNUF_NOT_DISABLEABLE;
        }
    }

    if (State & (PNP_DEVICE_REMOVED | PNP_DEVICE_DISABLED))
    {
        if (State & PNP_DEVICE_DISABLED)
            PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_HARDWARE_DISABLED);
        else
            PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_DEVICE_NOT_THERE);

        return STATUS_UNSUCCESSFUL;
    }

    if (State & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED)
    {
        IopResourceRequirementsChanged(DeviceObject, ((State & PNP_DEVICE_FAILED) != 0));
        return Status;
    }

    if (State & PNP_DEVICE_FAILED)
    {
        PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_FAILED_POST_START);
        return STATUS_UNSUCCESSFUL;
    }

    return Status;
}

static
BOOLEAN
NTAPI
PpCompareMultiLineIDs(
    _In_ PWCHAR Id1,
    _In_ PWCHAR Id2)
{

    for (;
         *Id2 != UNICODE_NULL;
         Id2 += wcslen(Id2) + 1, Id1 += wcslen(Id1) + 1)
    {
        if (*Id1 == UNICODE_NULL)
        {
            break;
        }

        if (_wcsicmp(Id2, Id1) != 0)
        {
            break;
        }
    }

    if (*Id2 != UNICODE_NULL || *Id1 != UNICODE_NULL)
    {
        DPRINT("PpCompareMultiLineIDs: different IDs\n");
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
NTAPI
PipProcessStartPhase3(
    _In_ PDEVICE_NODE DeviceNode)
{
    KEY_VALUE_PARTIAL_INFORMATION ValuePartialInfo;
    PKEY_VALUE_FULL_INFORMATION ValueFullInfo;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    PWCHAR HardwareIDs;
    PWCHAR CompatibleIDs;
    PWCHAR PrevIDs;
    ULONG ResultLength;
    ULONG HardwareIDsSize;
    ULONG CompatibleIDsSize;
    ULONG ConfigFlags;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PipProcessStartPhase3: DeviceNode - %p, Instance - %wZ\n",
           DeviceNode,  &DeviceNode->InstancePath);

    DeviceObject = DeviceNode->PhysicalDeviceObject;

    if (DeviceNode->Flags & DNF_IDS_QUERIED)
    {
        DPRINT("PipProcessStartPhase3: DeviceNode->Flags & DNF_IDS_QUERIED\n");
        goto Exit;
    }

    Status = PnpDeviceObjectToDeviceInstance(DeviceObject, &KeyHandle, KEY_READ);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipProcessStartPhase3: Status - %X\n", Status);
        goto Exit;
    }

    PpQueryID(DeviceNode, BusQueryHardwareIDs, &HardwareIDs, &HardwareIDsSize);
    PpQueryID(DeviceNode, BusQueryCompatibleIDs, &CompatibleIDs, &CompatibleIDsSize);

    if (!HardwareIDs && !CompatibleIDs)
    {
        ZwClose(KeyHandle);
        DeviceNode->Flags |= DNF_IDS_QUERIED;
        goto Exit;
    }

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    RtlInitUnicodeString(&ValueName, L"ConfigFlags");

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             &ValuePartialInfo,
                             sizeof(ValuePartialInfo) + sizeof(ULONG),
                             &ResultLength);

    if (NT_SUCCESS(Status) && ValuePartialInfo.Type == REG_DWORD)
    {
        ConfigFlags = ValuePartialInfo.Data[0];
    }
    else
    {
        ConfigFlags = 0;
    }

    if (HardwareIDs)
    {
        if (!(ConfigFlags & 0x400))
        {
            Status = IopGetRegistryValue(KeyHandle,
                                         L"HardwareID",
                                         &ValueFullInfo);

            if (NT_SUCCESS(Status))
            {
                if (ValueFullInfo->Type == REG_MULTI_SZ)
                {
                    PrevIDs = (PWCHAR)((ULONG_PTR)ValueFullInfo +
                                      ValueFullInfo->DataOffset);

                    if (PpCompareMultiLineIDs(HardwareIDs, PrevIDs))
                    {
                        DPRINT("PipProcessStartPhase3: HardwareID changed\n");
                        ConfigFlags |= 0x400;
                    }
                }

                ExFreePoolWithTag(ValueFullInfo, 'uspP');
            }
        }

        RtlInitUnicodeString(&ValueName, L"HardwareID");
        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_MULTI_SZ,
                      HardwareIDs,
                      HardwareIDsSize);

        ExFreePoolWithTag(HardwareIDs, 0);
    }

    if (CompatibleIDs)
    {
        if (!(ConfigFlags & 0x400))
        {
            Status = IopGetRegistryValue(KeyHandle,
                                         L"CompatibleIDs",
                                         &ValueFullInfo);

            if (NT_SUCCESS(Status))
            {
                if (ValueFullInfo->Type == REG_MULTI_SZ)
                {
                    PrevIDs = (PWCHAR)((ULONG_PTR)ValueFullInfo +
                                      ValueFullInfo->DataOffset);

                    if (PpCompareMultiLineIDs(HardwareIDs, PrevIDs))
                    {
                        DPRINT("PipProcessStartPhase3: CompatibleID changed\n");
                        ConfigFlags |= 0x400;
                    }
                }

                ExFreePoolWithTag(ValueFullInfo, 'uspP');
            }
        }

        RtlInitUnicodeString(&ValueName, L"CompatibleIDs");
        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_MULTI_SZ,
                      CompatibleIDs,
                      CompatibleIDsSize);

        ExFreePoolWithTag(CompatibleIDs, 0);
    }

    if (ConfigFlags & 0x400)
    {
        RtlInitUnicodeString(&ValueName, L"ConfigFlags");
        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_DWORD,
                      &ConfigFlags,
                      sizeof(ConfigFlags));
    }

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    ZwClose(KeyHandle);
    DeviceNode->Flags |= DNF_IDS_QUERIED;


Exit:

    if (DeviceNode->Flags & DNF_HAS_PROBLEM &&
        DeviceNode->Problem == CM_PROB_INVALID_DATA)
    {
        return STATUS_UNSUCCESSFUL;
    }

    DeviceNode->Flags |= DNF_REENUMERATE;

    IopQueryAndSaveDeviceNodeCapabilities(DeviceNode);

    Status = PiProcessQueryDeviceState(DeviceObject);

    DPRINT("PipProcessStartPhase3: FIXME PpSetPlugPlayEvent\n");
    //PpSetPlugPlayEvent(&GUID_DEVICE_ARRIVAL, DeviceNode->PhysicalDeviceObject);

    /* Report the device to the user-mode pnp manager */
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL,//GUID_DEVICE_ENUMERATED
                              &DeviceNode->InstancePath);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT("PipProcessStartPhase3: FIXME PpvUtilTestStartedPdoStack\n");
    //PpvUtilTestStartedPdoStack(DeviceObject);

    PipSetDevNodeState(DeviceNode, DeviceNodeStarted, NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PipEnumerateCompleted(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVICE_OBJECT DeviceObject;
    PEXTENDED_DEVOBJ_EXTENSION DeviceExt;
    PDEVICE_RELATIONS DeviceRelations;
    PDEVICE_NODE ChildDeviceNode;
    BOOLEAN RemovalChild;
    ULONG ix;
    BOOLEAN IsNoChildDeviceNode;

    PAGED_CODE();
    DPRINT("PipEnumerateCompleted: DeviceNode - %p\n", DeviceNode);

    if (!DeviceNode->OverUsed1.PendingDeviceRelations)
    {
        PipSetDevNodeState(DeviceNode, DeviceNodeStarted, NULL);
        return STATUS_SUCCESS;
    }

    for (ChildDeviceNode = DeviceNode->Child;
         ChildDeviceNode;
         ChildDeviceNode = ChildDeviceNode->Sibling)
    {
        ChildDeviceNode->Flags &= ~DNF_ENUMERATED;
    }

    for (ix = 0;
         ix < DeviceNode->OverUsed1.PendingDeviceRelations->Count;
         ix++)
    {
        DeviceRelations = DeviceNode->OverUsed1.PendingDeviceRelations;
        DeviceObject = DeviceRelations->Objects[ix];

        if (DeviceObject->Flags & DO_DEVICE_INITIALIZING)
        {
            DPRINT("PipEnumerateCompleted: DO_DEVICE_INITIALIZING! DeviceObject - %p\n",
                   DeviceObject);
        }

        DeviceExt = IoGetDevObjExtension(DeviceObject);

        if (DeviceExt->ExtensionFlags & DOE_DELETE_PENDING)
        {
            DPRINT1("PipEnumerateCompleted: FIXME dump\n");
            ASSERT(FALSE);

            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR,
                         4,
                         (ULONG_PTR)DeviceObject,
                         0,
                         0);
        }

        if (DeviceExt->DeviceNode)
        {
            ChildDeviceNode = DeviceExt->DeviceNode;
            ChildDeviceNode->Flags |= DNF_ENUMERATED;

            if (ChildDeviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED)
            {
                DPRINT1("PipEnumerateCompleted: FIXME PpProfileCancelTransitioningDock\n");
                ASSERT(FALSE);
            }

            ASSERT(!(ChildDeviceNode->Flags & DNF_DEVICE_GONE));

            ObDereferenceObject(DeviceObject);
        }
        else
        {
            ChildDeviceNode = PipAllocateDeviceNode(DeviceObject);
            DPRINT("PipEnumerateCompleted: ChildDeviceNode - %p\n", ChildDeviceNode);

            if (ChildDeviceNode)
            {
                ChildDeviceNode->Flags |= DNF_ENUMERATED;
                DeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;

                PpDevNodeInsertIntoTree(DeviceNode, ChildDeviceNode);

                DPRINT("PipEnumerateCompleted: FIXME PpSystemHiveTooLarge\n");
            }
            else
            {
                DPRINT1("PipEnumerateCompleted: Not allocated device node!\n");
                ObDereferenceObject(DeviceObject);
            }
        }
    }

    ExFreePoolWithTag(DeviceNode->OverUsed1.PendingDeviceRelations, 0);
    DeviceNode->OverUsed1.PendingDeviceRelations = NULL;

    RemovalChild = FALSE;
    ChildDeviceNode = DeviceNode->Child;

    for (IsNoChildDeviceNode = ChildDeviceNode == NULL;
         IsNoChildDeviceNode == FALSE;
         IsNoChildDeviceNode = ChildDeviceNode == NULL)
    {
        if (!(ChildDeviceNode->Flags & DNF_ENUMERATED) &&
            !(ChildDeviceNode->Flags & DNF_DEVICE_GONE))
        {
            ChildDeviceNode->Flags |= DNF_DEVICE_GONE;
            PipRequestDeviceRemoval(ChildDeviceNode, TRUE, CM_PROB_DEVICE_NOT_THERE);
            RemovalChild = TRUE;
        }

        ChildDeviceNode = ChildDeviceNode->Sibling;
    }

    ASSERT(DeviceNode->State == DeviceNodeEnumerateCompletion);
    PipSetDevNodeState(DeviceNode, DeviceNodeStarted, NULL);

    if (RemovalChild && DeviceNode != IopRootDeviceNode)
    {
        return STATUS_PNP_RESTART_ENUMERATION;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PpQueryDeviceID(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PWCHAR *OutFullDeviceID,
    _Out_ PWCHAR *OutDeviceID)
{
    PWCHAR Id;
    PWCHAR Separator;
    ULONG IdLength;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpQueryDeviceID: DeviceNode - %p\n", DeviceNode);

    *OutFullDeviceID = NULL;
    *OutDeviceID = NULL;

    Status = PpQueryID(DeviceNode, BusQueryDeviceID, &Id, &IdLength);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PpQueryDeviceID: Status - %p\n", Status);
        ASSERT(Id == NULL && IdLength == 0);
        return Status;
    }

    ASSERT(Id && IdLength);

    *OutFullDeviceID = Id;

    Separator = wcschr(Id, L'\\');
    ASSERT(Separator);
    *Separator = UNICODE_NULL;

    *OutDeviceID = Separator + 1;

    return Status;
}

NTSTATUS
NTAPI
PipMakeGloballyUniqueId(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PWCHAR InstanceID,
    _Out_ PWCHAR *OutUniqueId)
{
    UNICODE_STRING EnumKeyName = RTL_CONSTANT_STRING(ENUM_ROOT);
    UNICODE_STRING ValueName;
    KEY_VALUE_PARTIAL_INFORMATION KeyValue;
    PKEY_VALUE_PARTIAL_INFORMATION PrefixBuffer;
    PDEVICE_NODE ParentNode;
    PWCHAR UniqueIdBuffer;
    PWSTR UniqueIdString = NULL;
    PWCHAR UniqueIdStringEnd;
    HANDLE Handle;
    HANDLE KeyHandle;
    ULONG InstanceCounter;
    ULONG ResultLength;
    ULONG UniqueParentID;
    ULONG InstanceIdLen;
    PWSTR pChar;
    ULONG Key;
    ULONG Hash;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PipMakeGloballyUniqueId: InstanceID - %S\n", InstanceID);

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    ParentNode = (IopGetDeviceNode(DeviceObject))->Parent;

    Status = IopOpenRegistryKeyEx(&Handle,
                                  NULL,
                                  &EnumKeyName,
                                  KEY_READ | KEY_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PipMakeGloballyUniqueId: Status - %X\n", Status);

        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();

        *OutUniqueId = NULL;
        return Status;
    }

    Status = IopOpenRegistryKeyEx(&KeyHandle,
                                  Handle,
                                  &ParentNode->InstancePath,
                                  KEY_READ | KEY_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipMakeGloballyUniqueId: Status - %X\n", Status);
        goto Exit;
    }

    RtlInitUnicodeString(&ValueName, L"UniqueParentID");

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             &KeyValue,
                             sizeof(KEY_VALUE_PARTIAL_INFORMATION),
                             &ResultLength);

    if (NT_SUCCESS(Status))
    {
        ASSERT(KeyValue.Type == REG_DWORD);
        ASSERT(KeyValue.DataLength == sizeof(ULONG));

        if (KeyValue.Type != REG_DWORD ||
            KeyValue.DataLength != sizeof(ULONG))
        {
            Status = STATUS_INVALID_PARAMETER;
            ZwClose(KeyHandle);
            goto Exit;
        }

        UniqueParentID = KeyValue.Data[0];

        UniqueIdString = ExAllocatePoolWithTag(PagedPool,
                                               9 * sizeof(WCHAR),
                                               'nepP');
        if (!UniqueIdString)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ZwClose(KeyHandle);
            goto Exit;
        }

        RtlStringCbPrintfW(UniqueIdString,
                           9 * sizeof(WCHAR),
                           L"%x",
                           UniqueParentID);

        goto MakeUniqueId;
    }

    /* 
        Format key "ParentIdPrefix" ("%x&%x&%x"):
        [ParentNode->Level] + "&" + [Hash] + "&" + [InstanceCounter]
                      8        1      8       1        8
    */

    ResultLength = 66; // (sizeof(KEY_VALUE_PARTIAL_INFORMATION) - (1+3)) + (8+1+8+1+8)*2 + sizeof(UNICODE_NULL)

    PrefixBuffer = ExAllocatePoolWithTag(PagedPool, ResultLength, 'nepP');

    if (!PrefixBuffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        ZwClose(KeyHandle);
        goto Exit;
    }

    RtlInitUnicodeString(&ValueName, L"ParentIdPrefix");

    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             PrefixBuffer,
                             ResultLength,
                             &ResultLength);

    if (!NT_SUCCESS(Status))
    {
        /* 
            Format key "NextParentID" ("%s.%x.%x"):
            "NextParentID" + "." + [Hash] + "." + [ParentNode->Level]
                              1      8       1        8
        */

        ResultLength = wcslen(L"NextParentID") + 19; // (1+8+1+8) + 1

        Status = RtlUpcaseUnicodeString(&ValueName,
                                        &ParentNode->InstancePath,
                                        TRUE);
        if (!NT_SUCCESS(Status))
        {
            ZwClose(KeyHandle);
            goto Exit;
        }

        /* Calculate hash for UniqueId */
        Key = 0;

        for (pChar = ValueName.Buffer;
             pChar < &ValueName.Buffer[ValueName.Length / sizeof(WCHAR)];
             pChar++)
        {
            Key = *pChar + 37 * Key; // ?37?
        }

        Hash = abs(CMP_HASH_IRRATIONAL * Key) % CMP_HASH_PRIME;

        RtlFreeUnicodeString(&ValueName);

        UniqueIdString = ExAllocatePoolWithTag(PagedPool,
                                               ResultLength * sizeof(WCHAR),
                                               'nepP');
        if (!UniqueIdString)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ZwClose(KeyHandle);
            goto Exit;
        }

        RtlStringCbPrintfW(UniqueIdString,
                           ResultLength * sizeof(WCHAR),
                           L"%s.%x.%x",
                           L"NextParentID",
                           Hash,
                           ParentNode->Level);

        RtlInitUnicodeString(&ValueName, UniqueIdString);

        Status = ZwQueryValueKey(Handle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 &KeyValue,
                                 sizeof(KEY_VALUE_PARTIAL_INFORMATION),
                                 &ResultLength);

        if (NT_SUCCESS(Status) &&
            KeyValue.Type == REG_DWORD &&
            KeyValue.DataLength == sizeof(ULONG))
        {
            InstanceCounter = KeyValue.Data[0];
        }
        else
        {
            InstanceCounter = 0;
        }

        InstanceCounter++;

        Status = ZwSetValueKey(Handle,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &InstanceCounter,
                               sizeof(ULONG));

        if (!NT_SUCCESS(Status))
        {
            ZwClose(KeyHandle);
            goto Exit;
        }

        InstanceCounter--;

        RtlStringCchPrintfExW(UniqueIdString,
                              ResultLength,
                              &UniqueIdStringEnd,
                              NULL,
                              0,
                              L"%x&%x&%x",
                              ParentNode->Level,
                              Hash,
                              InstanceCounter);

        ResultLength = (UniqueIdStringEnd - UniqueIdString) + 1;

        RtlInitUnicodeString(&ValueName, L"ParentIdPrefix");

        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_SZ,
                               UniqueIdString,
                               ResultLength * sizeof(WCHAR));

        if (!NT_SUCCESS(Status))
        {
            ZwClose(KeyHandle);
            goto Exit;
        }
    }
    else
    {
        if (PrefixBuffer->Type != REG_SZ)
        {
            ASSERT(PrefixBuffer->Type == REG_SZ);

            if (PrefixBuffer->Type != REG_SZ)
            {
                Status = STATUS_INVALID_PARAMETER;
                ZwClose(KeyHandle);
                goto Exit;
            }
        }

        UniqueIdString = ExAllocatePoolWithTag(PagedPool,
                                               PrefixBuffer->DataLength,
                                               'nepP');

        if (!UniqueIdString)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            ZwClose(KeyHandle);
            goto Exit;
        }

        RtlStringCbCopyW(UniqueIdString,
                         PrefixBuffer->DataLength,
                         (PWSTR)PrefixBuffer->Data);
    }

MakeUniqueId:

    if (InstanceID)
    {
        InstanceIdLen = wcslen(InstanceID);
    }
    else
    {
        InstanceIdLen = 0;
    }

    ResultLength = wcslen(UniqueIdString) + InstanceIdLen + 2;

    UniqueIdBuffer = ExAllocatePoolWithTag(PagedPool,
                                           ResultLength * sizeof(WCHAR),
                                           'nepP');
    if (!UniqueIdBuffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (InstanceID)
    {
        RtlStringCchPrintfW(UniqueIdBuffer,
                            ResultLength,
                            L"%s&%s",
                            UniqueIdString,
                            InstanceID);
    }
    else
    {
        RtlStringCchCopyW(UniqueIdBuffer,
                          ResultLength,
                          UniqueIdString);
    }

    ZwClose(KeyHandle);

Exit:

    ZwClose(Handle);

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    if (PrefixBuffer)
    {
        ExFreePoolWithTag(PrefixBuffer, 'nepP');
    }

    if (UniqueIdString)
    {
        ExFreePoolWithTag(UniqueIdString, 'nepP');
    }

    *OutUniqueId = UniqueIdBuffer;

    return Status;
}

NTSTATUS
NTAPI
PiBuildDeviceNodeInstancePath(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PWCHAR DeviceType,
    _In_ PWCHAR DeviceID,
    _In_ PWCHAR InstanceID)
{
    SIZE_T Lenght;
    PWCHAR InstancePath;

    PAGED_CODE();
    DPRINT("PiBuildDeviceNodeInstancePath: DeviceNode - %p, DeviceType - %S, DeviceID - %S, InstanceID - %S\n",
           DeviceNode, DeviceType, DeviceID, InstanceID);

    if (!DeviceType || !DeviceID || !InstanceID)
    {
        DPRINT("PiBuildDeviceNodeInstancePath: !DeviceType || !DeviceID || !InstanceID\n");

        ASSERT((DeviceNode->Flags & DNF_HAS_PROBLEM) != 0);
        ASSERT(DeviceNode->Problem == CM_PROB_INVALID_DATA ||
               DeviceNode->Problem == CM_PROB_OUT_OF_MEMORY ||
               DeviceNode->Problem == CM_PROB_REGISTRY);

        return STATUS_UNSUCCESSFUL;
    }

    Lenght = (wcslen(DeviceType) + 1 +
              wcslen(DeviceID) + 1 +
              wcslen(InstanceID) + 1) * sizeof(WCHAR);

    InstancePath = ExAllocatePoolWithTag(PagedPool, Lenght, 'nepP');

    if (!InstancePath)
    {
        DPRINT1("PiBuildDeviceNodeInstancePath: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlStringCbPrintfW(InstancePath,
                       Lenght,
                       L"%s\\%s\\%s",
                       DeviceType,
                       DeviceID,
                       InstanceID);

    if (DeviceNode->InstancePath.Buffer)
    {
        IopCleanupDeviceRegistryValues(&DeviceNode->InstancePath);
        ExFreePoolWithTag(DeviceNode->InstancePath.Buffer, 'nepP');
    }

    RtlInitUnicodeString(&DeviceNode->InstancePath, InstancePath);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiCreateDeviceInstanceKey(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PHANDLE OutKeyHandle,
    _Out_ PULONG OutDisposition)
{
    UNICODE_STRING EnumKeyName = RTL_CONSTANT_STRING(ENUM_ROOT);
    UNICODE_STRING ValueName;
    PKEY_VALUE_FULL_INFORMATION KeyInfo;
    HANDLE EnumHandle;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PiCreateDeviceInstanceKey: DeviceNode %p\n", DeviceNode);

    *OutKeyHandle = NULL;
    *OutDisposition = 0;

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    Status = IopOpenRegistryKeyEx(&EnumHandle,
                                  NULL,
                                  &EnumKeyName,
                                  KEY_ALL_ACCESS);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PiCreateDeviceInstanceKey: Status - %X\n", Status);
        ASSERT(EnumHandle != NULL);
        goto Exit;
    }

    Status = IopCreateRegistryKeyEx(OutKeyHandle,
                                    EnumHandle,
                                    &DeviceNode->InstancePath,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    OutDisposition);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PiCreateDeviceInstanceKey: Status - %X\n", Status);
        ASSERT(*OutKeyHandle != NULL);
        goto Exit;
    }

    if (*OutDisposition == REG_CREATED_NEW_KEY)
    {
        goto Exit;
    }

    KeyInfo = NULL;
    IopGetRegistryValue(*OutKeyHandle, L"Migrated", &KeyInfo);

    if (!KeyInfo)
    {
        goto Exit;
    }

    if (KeyInfo->Type == REG_DWORD &&
        KeyInfo->DataLength == sizeof(ULONG) &&
        *(PULONG)((ULONG_PTR)&KeyInfo->TitleIndex + KeyInfo->DataOffset))
    {
        *OutDisposition = REG_CREATED_NEW_KEY;
    }

    RtlInitUnicodeString(&ValueName, L"Migrated");
    ZwDeleteValueKey(*OutKeyHandle, &ValueName);

    ExFreePoolWithTag(KeyInfo, 'uspP');

Exit:

    if (EnumHandle)
    {
        ZwClose(EnumHandle);
    }

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    return Status;
}

VOID
NTAPI
PpMarkDeviceStackStartPending(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN MarkType)
{
    PDEVICE_OBJECT Device;
    PEXTENDED_DEVOBJ_EXTENSION DevObjExtension;
    KSPIN_LOCK DeviceStackLock;
    KIRQL OldIrql;

    DPRINT("PpMarkDeviceStackStartPending: DeviceObject %p\n", DeviceObject);

    KeAcquireSpinLock(&DeviceStackLock, &OldIrql);

    for (Device = DeviceObject;
         Device;
         Device = Device->AttachedDevice)
    {
        DevObjExtension = IoGetDevObjExtension(Device);

        if (MarkType)
        {
            DevObjExtension->ExtensionFlags |= DOE_START_PENDING;
        }
        else
        {
            DevObjExtension->ExtensionFlags &= ~DOE_START_PENDING;
        }
    }

    KeReleaseSpinLock(&DeviceStackLock, OldIrql);
}

NTSTATUS
NTAPI
PiQueryResourceRequirements(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ HANDLE KeyHandle)
{
    PIO_RESOURCE_REQUIREMENTS_LIST IoResource;
    UNICODE_STRING ValueName;
    ULONG ListSize;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PiQueryResourceRequirements: DeviceNode - %p, KeyHandle - %p\n",
           DeviceNode, KeyHandle);

    Status = PpIrpQueryResourceRequirements(DeviceNode->PhysicalDeviceObject,
                                            &IoResource);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PiQueryResourceRequirements: Status - %X\n", Status);
        ASSERT(IoResource == NULL);
        IoResource = NULL;
    }

    if (IoResource)
    {
        ListSize = IoResource->ListSize;
    }
    else
    {
        ListSize = 0;
    }

    if (!KeyHandle)
    {
        if (IoResource)
        {
            ExFreePool(IoResource);
        }

        DPRINT("PiQueryResourceRequirements: Status - %X\n", Status);
        return Status;
    }

    RtlInitUnicodeString(&ValueName, L"BasicConfigVector");

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    if (IoResource)
    {
        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_RESOURCE_REQUIREMENTS_LIST,
                      IoResource,
                      ListSize);

        DeviceNode->Flags |= DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
        DeviceNode->ResourceRequirements = IoResource;
    }
    else
    {
        ZwDeleteValueKey(KeyHandle, &ValueName);
    }

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    if (DeviceNode->ResourceRequirements)
    {
        DPRINT("PiQueryResourceRequirements: DeviceNode->ResourceRequirements - %p\n",
               DeviceNode->ResourceRequirements);

        PipDumpResourceRequirementsList(DeviceNode->ResourceRequirements, 1);
    }

    return Status;
}

NTSTATUS
NTAPI
PpQueryBusInformation(
    _In_ PDEVICE_NODE DeviceNode)
{
    PPNP_BUS_INFORMATION BusInfo;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpQueryBusInformation: DeviceNode - %p\n", DeviceNode);

    Status = PpIrpQueryBusInformation(DeviceNode->PhysicalDeviceObject, &BusInfo);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(BusInfo == NULL);

        DeviceNode->ChildBusTypeIndex = -1;
        DeviceNode->ChildInterfaceType = InterfaceTypeUndefined;
        DeviceNode->ChildBusNumber = 0xFFFFFFF0;
    }
    else
    {
        ASSERT(BusInfo);

        DeviceNode->ChildBusTypeIndex = IopGetBusTypeGuidIndex(&BusInfo->BusTypeGuid);
        DeviceNode->ChildInterfaceType = BusInfo->LegacyBusType;
        DeviceNode->ChildBusNumber = BusInfo->BusNumber;

        ExFreePool(BusInfo);
    }

    return Status;
}

NTSTATUS
NTAPI
PiQueryAndAllocateBootResources(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ HANDLE KeyHandle)
{
    PCM_RESOURCE_LIST CmResource = NULL;
    UNICODE_STRING ValueName;
    ULONG CmLength = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    ASSERT(DeviceNode);

    DPRINT("PiQueryAndAllocateBootResources: DeviceNode - %p, DeviceNode->BootResources - %p\n",
           DeviceNode, DeviceNode->BootResources);

    if (DeviceNode->BootResources == NULL)
    {
        Status = PpIrpQueryResources(DeviceNode->PhysicalDeviceObject,
                                     &CmResource,
                                     &CmLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("PiQueryAndAllocateBootResources: Status - %X, DeviceNode->BootResources - %p\n",
                   Status, DeviceNode->BootResources);

            ASSERT(CmResource == NULL && CmLength == 0);

            CmResource = NULL;
            CmLength = 0;
        }
    }
    else
    {
        DPRINT("PiQueryAndAllocateBootResources: %S already has BOOT config in PiQueryAndAllocateBootResources!\n",
               DeviceNode->InstancePath.Buffer);
    }

    if (!KeyHandle || DeviceNode->BootResources)
    {
        if (CmResource)
        {
            ExFreePoolWithTag(CmResource, 0);
        }

        DPRINT("PiQueryAndAllocateBootResources: DeviceNode->BootResources - %X\n",
               DeviceNode->BootResources);

        return Status;
    }
    else
    {
        DPRINT("PiQueryAndAllocateBootResources: KeyHandle - %X\n", KeyHandle);
    }

    RtlInitUnicodeString(&ValueName, L"BootConfig");

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    if (CmResource)
    {
        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_RESOURCE_LIST,
                      CmResource,
                      CmLength);
    }
    else
    {
        ZwDeleteValueKey(KeyHandle, &ValueName);
    }

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    if (!CmResource)
    {
        return Status;
    }

    Status = IopAllocateBootResourcesRoutine(ArbiterRequestPnpEnumerated,
                                             DeviceNode->PhysicalDeviceObject,
                                             CmResource);

    DPRINT("PiQueryAndAllocateBootResources: DeviceNode->BootResources - %X, Status - %X\n",
           DeviceNode->BootResources, Status);

    if (DeviceNode->BootResources)
    {
        PipDumpCmResourceList(DeviceNode->BootResources, 1);
    }

    if (NT_SUCCESS(Status))
    {
        DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
    }

    if (CmResource)
    {
        ExFreePoolWithTag(CmResource, 0);
    }

    return Status;
}

NTSTATUS
NTAPI
PiCriticalQueryRegistryValueCallback(
    _In_ PWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID ValueData,
    _In_ ULONG ValueLength,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID EntryContext)
{
    /* "Security" key */

    DPRINT("PiCriticalQueryRegistryValueCallback: ValueName - %S\n", ValueName);

    if (ValueType != REG_BINARY)
    {
        DPRINT("PiCriticalQueryRegistryValueCallback: ValueType - %X\n", ValueType);
        return STATUS_SUCCESS;
    }

    if (!ValueLength)
    {
        DPRINT("PiCriticalQueryRegistryValueCallback: ValueLength == 0\n");
        return STATUS_SUCCESS;
    }

    if (!ValueData)
    {
        DPRINT("PiCriticalQueryRegistryValueCallback: ValueData == NULL\n");
        return STATUS_SUCCESS;
    }

    DPRINT1("PiCriticalQueryRegistryValueCallback: FIXME ...\n");
    ASSERT(FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiCriticalCopyCriticalDeviceProperties(
    _In_ HANDLE KeyHandle,
    _In_ HANDLE CriticalHandle)
{
    PKEY_VALUE_FULL_INFORMATION KeyValueFullInfo;
    RTL_QUERY_REGISTRY_TABLE QueryTable[9];
    PVOID SecurityContext = NULL;
    UNICODE_STRING LowerFiltersString;
    UNICODE_STRING UpperFiltersString;
    UNICODE_STRING ClassGuidString;
    UNICODE_STRING ServiceString;
    UNICODE_STRING ValueName;
    ULONG DataSize = 0;
    ULONG defaultData = 0;
    ULONG DeviceCharacteristicsContext = 0;
    ULONG ExclusiveData = 0;
    ULONG DeviceTypeData = 0;
    NTSTATUS status;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PiCriticalCopyCriticalDeviceProperties()\n");

    if (!KeyHandle || !CriticalHandle)
    {
        DPRINT1("PiCriticalCopyCriticalDeviceProperties: %p, %p\n", KeyHandle, CriticalHandle);
        ASSERT(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&QueryTable, sizeof(QueryTable));

    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"Service";
    QueryTable[0].EntryContext = &ServiceString;
    QueryTable[0].DefaultType = REG_SZ;
    QueryTable[0].DefaultData = L"";
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].Name = L"ClassGUID";
    QueryTable[1].EntryContext = &ClassGuidString;
    QueryTable[1].DefaultType = REG_SZ;
    QueryTable[1].DefaultData = L"";
    QueryTable[1].DefaultLength = 0;

    QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT + RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[2].Name = L"LowerFilters";
    QueryTable[2].EntryContext = &LowerFiltersString;
    QueryTable[2].DefaultType = REG_MULTI_SZ;
    QueryTable[2].DefaultData = L"";
    QueryTable[2].DefaultLength = 0;

    QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT + RTL_QUERY_REGISTRY_NOEXPAND;
    QueryTable[3].Name = L"UpperFilters";
    QueryTable[3].EntryContext = &UpperFiltersString;
    QueryTable[3].DefaultType = REG_MULTI_SZ;
    QueryTable[3].DefaultData = L"";
    QueryTable[3].DefaultLength = 0;

    QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[4].Name = L"DeviceType";
    QueryTable[4].EntryContext = &DeviceTypeData;
    QueryTable[4].DefaultType = REG_DWORD;
    QueryTable[4].DefaultData = &defaultData;
    QueryTable[4].DefaultLength = sizeof(defaultData);

    QueryTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[5].Name = L"Exclusive";
    QueryTable[5].EntryContext = &ExclusiveData;
    QueryTable[5].DefaultType = REG_DWORD;
    QueryTable[5].DefaultData = &defaultData;
    QueryTable[5].DefaultLength = sizeof(defaultData);

    QueryTable[6].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[6].Name = L"DeviceCharacteristics";
    QueryTable[6].EntryContext = &DeviceCharacteristicsContext;
    QueryTable[6].DefaultType = REG_DWORD;
    QueryTable[6].DefaultData = &defaultData;
    QueryTable[6].DefaultLength = sizeof(defaultData);

    QueryTable[7].QueryRoutine = PiCriticalQueryRegistryValueCallback;
    QueryTable[7].Flags = 0;
    QueryTable[7].Name = L"Security";
    QueryTable[7].EntryContext = &SecurityContext;
    QueryTable[7].DefaultType = REG_BINARY;
    QueryTable[7].DefaultData = 0;
    QueryTable[7].DefaultLength = 0;

    RtlZeroMemory(&ServiceString, sizeof(ServiceString));
    RtlZeroMemory(&ClassGuidString, sizeof(ClassGuidString));
    RtlZeroMemory(&LowerFiltersString, sizeof(LowerFiltersString));
    RtlZeroMemory(&UpperFiltersString, sizeof(UpperFiltersString));

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                                    (PCWSTR)CriticalHandle,
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    if (!ServiceString.Length && ServiceString.Buffer)
    {
        RtlFreeUnicodeString(&ServiceString);
    }

    if (!ClassGuidString.Length && ClassGuidString.Buffer)
    {
        RtlFreeUnicodeString(&ClassGuidString);
    }

    if (UpperFiltersString.Length <= sizeof(WCHAR) && UpperFiltersString.Buffer)
    {
        RtlFreeUnicodeString(&UpperFiltersString);
    }

    if (LowerFiltersString.Length <= sizeof(WCHAR) && LowerFiltersString.Buffer)
    {
        RtlFreeUnicodeString(&LowerFiltersString);
    }

    DPRINT("PiCriticalCopyCriticalDeviceProperties: Setup critical service\n");

    if (!ServiceString.Buffer)
    {
        DPRINT1("PiCriticalCopyCriticalDeviceProperties: ServiceString.Buffer == NULL\n");
        ASSERT(ServiceString.Buffer);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    RtlInitUnicodeString(&ValueName, L"Service");

    DPRINT("PiCriticalCopyCriticalDeviceProperties: ServiceString - %wZ, ValueName - %wZ\n",
           &ServiceString, &ValueName);

    Status = ZwSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           ServiceString.Buffer,
                           ServiceString.Length + sizeof(WCHAR));

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PiCriticalCopyCriticalDeviceProperties: Status - %X\n", Status);
        goto Exit;
    }

    if (ClassGuidString.Buffer)
    {
        RtlInitUnicodeString(&ValueName, L"ClassGUID");

        DPRINT("PiCriticalCopyCriticalDeviceProperties: ClassGuidString - %wZ, ValueName - %wZ\n",
               &ClassGuidString, &ValueName);

        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_SZ,
                      ClassGuidString.Buffer,
                      ClassGuidString.Length + sizeof(WCHAR));
    }

    if (LowerFiltersString.Buffer)
    {
        RtlInitUnicodeString(&ValueName, L"LowerFilters");

        DPRINT("PiCriticalCopyCriticalDeviceProperties: LowerFiltersString - %wZ, ValueName - %wZ\n",
               &LowerFiltersString, &ValueName);

        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_MULTI_SZ,
                      LowerFiltersString.Buffer,
                      LowerFiltersString.Length);
    }

    if (UpperFiltersString.Buffer)
    {
        RtlInitUnicodeString(&ValueName, L"UpperFilters");

        DPRINT("PiCriticalCopyCriticalDeviceProperties: UpperFiltersString - %wZ, ValueName - %wZ\n",
               &UpperFiltersString, &ValueName);

        ZwSetValueKey(KeyHandle,
                      &ValueName,
                      0,
                      REG_MULTI_SZ,
                      UpperFiltersString.Buffer,
                      UpperFiltersString.Length);
    }

    if (DeviceTypeData)
    {
        RtlInitUnicodeString(&ValueName, L"DeviceType");

        DPRINT("PiCriticalCopyCriticalDeviceProperties: DeviceType - %X, ValueName - %wZ\n",
               &DeviceTypeData, &ValueName);

        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &DeviceTypeData,
                               sizeof(DeviceTypeData));

        if (!NT_SUCCESS(Status))
        {
            DPRINT("PiCriticalCopyCriticalDeviceProperties: Status - %X\n", Status);
            goto Exit;
        }
    }

    if (ExclusiveData)
    {
        RtlInitUnicodeString(&ValueName, L"Exclusive");

        DPRINT("PiCriticalCopyCriticalDeviceProperties: ValueName - %wZ\n", &ValueName);

        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &ExclusiveData,
                               sizeof(ExclusiveData));

        if (!NT_SUCCESS(Status))
        {
            DPRINT("PiCriticalCopyCriticalDeviceProperties: Status - %X\n", Status);
            goto Exit;
        }
    }

    if (DeviceCharacteristicsContext)
    {
        RtlInitUnicodeString(&ValueName, L"DeviceCharacteristics");

        DPRINT("PiCriticalCopyCriticalDeviceProperties: ValueName - %wZ\n", &ValueName);

        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &DeviceCharacteristicsContext,
                               sizeof(DeviceCharacteristicsContext));

        if (!NT_SUCCESS(Status))
        {
            DPRINT("PiCriticalCopyCriticalDeviceProperties: Status - %X\n", Status);
            goto Exit;
        }
    }

    if (SecurityContext)
    {
        RtlInitUnicodeString(&ValueName, L"Security");

        DPRINT("PiCriticalCopyCriticalDeviceProperties: ValueName - %wZ\n", &ValueName);

        Status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               0,
                               REG_DWORD,
                               SecurityContext,
                               DataSize);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("PiCriticalCopyCriticalDeviceProperties: Status - %X\n", Status);
            goto Exit;
        }
    }

    KeyValueFullInfo = NULL;

    status = IopGetRegistryValue(CriticalHandle,
                                 L"PreservePreInstall",
                                 &KeyValueFullInfo);

    if (!NT_SUCCESS(status))
    {
        DPRINT("PiCriticalCopyCriticalDeviceProperties: Status - %X\n", Status);
        goto Exit;
    }

    ASSERT(KeyValueFullInfo);
    ASSERT(KeyValueFullInfo->Type == REG_DWORD);
    ASSERT(KeyValueFullInfo->DataLength == sizeof(ULONG));

    if (KeyValueFullInfo->Type == REG_DWORD &&
        KeyValueFullInfo->DataLength == sizeof(ULONG))
    {
        RtlInitUnicodeString(&ValueName, L"PreservePreInstall");

        status = ZwSetValueKey(KeyHandle,
                               &ValueName,
                               KeyValueFullInfo->TitleIndex,
                               KeyValueFullInfo->Type,
                               (PUCHAR)KeyValueFullInfo + KeyValueFullInfo->DataOffset,
                               KeyValueFullInfo->DataLength);

        if (!NT_SUCCESS(status))
        {
            DPRINT("PiCriticalCopyCriticalDeviceProperties: Status - %X\n", Status);
        }
    }

    ExFreePoolWithTag(KeyValueFullInfo, 'uspP');

Exit:

    RtlFreeUnicodeString(&ServiceString);
    RtlFreeUnicodeString(&ClassGuidString);
    RtlFreeUnicodeString(&LowerFiltersString);
    RtlFreeUnicodeString(&UpperFiltersString);

    if (SecurityContext)
    {
        ExFreePool(SecurityContext);
    }

    return Status;
}

BOOLEAN
NTAPI
PiCriticalCallbackVerifyCriticalEntry(HANDLE KeyHandle)
{
    PKEY_VALUE_FULL_INFORMATION KeyValueFullInfo;
    ULONG Type;
    ULONG DataLength;
    ULONG ClassGUIDLenght;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PiCriticalCallbackVerifyCriticalEntry()\n");

    if (!KeyHandle)
    {
        DPRINT("PiCriticalCallbackVerifyCriticalEntry: KeyHandle - NULL\n");
        return FALSE;
    }

    KeyValueFullInfo = NULL;

    Status = IopGetRegistryValue(KeyHandle, L"Service", &KeyValueFullInfo);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PiCriticalCallbackVerifyCriticalEntry: Status - %X\n", Status);
        ASSERT(KeyValueFullInfo == NULL);
        return FALSE;
    }

    ASSERT(KeyValueFullInfo);

    Type = KeyValueFullInfo->Type;
    DataLength = KeyValueFullInfo->DataLength;

    ExFreePoolWithTag(KeyValueFullInfo, 'uspP');

    if (Type != REG_SZ || DataLength <= sizeof(WCHAR))
    {
        DPRINT("PiCriticalCallbackVerifyCriticalEntry: Type - %X, DataLength - %X\n",
               Type, DataLength);

        return FALSE;
    }

    KeyValueFullInfo = NULL;

    Status = IopGetRegistryValue(KeyHandle, L"ClassGUID", &KeyValueFullInfo);

    if (!NT_SUCCESS(Status))
    {
        ASSERT(KeyValueFullInfo == NULL);

        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            Status = STATUS_SUCCESS;
        }

        DPRINT("PiCriticalCallbackVerifyCriticalEntry: Status %X\n", Status);
        return FALSE;
    }

    ASSERT(KeyValueFullInfo != NULL);

    Type = KeyValueFullInfo->Type;
    DataLength = KeyValueFullInfo->DataLength;

    ExFreePoolWithTag(KeyValueFullInfo, 'uspP');

    ClassGUIDLenght = strlen("{00000000-0000-0000-0000-000000000000}") * sizeof(WCHAR);

    if (Type != REG_SZ ||
        (DataLength > sizeof(WCHAR) && DataLength < ClassGUIDLenght))
    {
        DPRINT("PiCriticalCallbackVerifyCriticalEntry: Type - %X, DataLength - %X\n",
               Type, DataLength);

        Status = STATUS_UNSUCCESSFUL;
    }

    return NT_SUCCESS(Status);
}

NTSTATUS
NTAPI
PiCriticalOpenFirstMatchingSubKey(
    _In_ PWSTR IdString,
    _In_ HANDLE DatabaseRootHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PIP_CRITICAL_CALLBACK_VERIFY_CRITICAL_ENTRY CallbackRoutine,
    _Out_ PHANDLE MatchingKeyHandle)
{
    UNICODE_STRING MatchingString;
    PWCHAR String;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PiCriticalOpenFirstMatchingSubKey: IdString - %S\n", IdString);

    if (!IdString || !DatabaseRootHandle || !MatchingKeyHandle)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *MatchingKeyHandle = NULL;
    Status = STATUS_OBJECT_NAME_NOT_FOUND;

    if (!*IdString)
    {
        return Status;
    }

    for (String = IdString;
         *String;
         String += wcslen(String) + 1)
    {
        RtlInitUnicodeString(&MatchingString, String);

        Status = IopOpenRegistryKeyEx(MatchingKeyHandle,
                                      DatabaseRootHandle,
                                      &MatchingString,
                                      DesiredAccess);
        if (!NT_SUCCESS(Status))
        {
            ASSERT(*MatchingKeyHandle == NULL);
            *MatchingKeyHandle = NULL;
            continue;
        }

        ASSERT(*MatchingKeyHandle != NULL);

        if (CallbackRoutine)
        {
            if (CallbackRoutine(*MatchingKeyHandle))
            {
                DPRINT("PiCriticalOpenFirstMatchingSubKey: Callback return TRUE\n");
                break;
            }
        }
        else
        {
            DPRINT("PiCriticalOpenFirstMatchingSubKey: CallbackRoutine == NULL\n");
            break;
        }

        Status = STATUS_OBJECT_NAME_NOT_FOUND;

        ZwClose(*MatchingKeyHandle);
        *MatchingKeyHandle = NULL;
    }

    if (!NT_SUCCESS(Status))
    {
        ASSERT(*MatchingKeyHandle == NULL);
    }
    else if (!*MatchingKeyHandle)
    {
        ASSERT(*MatchingKeyHandle != NULL);
    }

    return Status;
}

NTSTATUS
NTAPI
PiCriticalOpenCriticalDeviceKey(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ HANDLE Handle,
    _In_ PHANDLE CriticalDeviceEntryHandle)
{
    PDEVICE_OBJECT DeviceObject;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    HANDLE DatabaseRootHandle;
    HANDLE DeviceInstanceHandle;
    UNICODE_STRING CddNameString;
    PWSTR IdNameString[2];
    PWCHAR IdBuffer;
    ULONG ix;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PiCriticalOpenCriticalDeviceKey: DeviceNode - %p\n", DeviceNode);

    if (!DeviceNode || !CriticalDeviceEntryHandle)
    {
        DPRINT("PiCriticalOpenCriticalDeviceKey: CriticalDeviceEntryHandle - %p\n",
               CriticalDeviceEntryHandle);

        return STATUS_INVALID_PARAMETER;
    }

    *CriticalDeviceEntryHandle = NULL;

    if (Handle)
    {
        DatabaseRootHandle = Handle;
    }
    else
    {
        RtlInitUnicodeString(&CddNameString,
                             L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\CriticalDeviceDatabase");

        Status = IopOpenRegistryKeyEx(&DatabaseRootHandle,
                                      NULL,
                                      &CddNameString,
                                      KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("PiCriticalOpenCriticalDeviceKey: Status - %X\n", Status);
            return Status;
        }
    }

    ASSERT(DatabaseRootHandle);

    DeviceObject = DeviceNode->PhysicalDeviceObject;
    DeviceInstanceHandle = NULL;

    Status = PnpDeviceObjectToDeviceInstance(DeviceObject,
                                             &DeviceInstanceHandle,
                                             KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PiCriticalOpenCriticalDeviceKey: Status - %X\n", Status);
        ASSERT(!DeviceInstanceHandle);
        goto Exit;
    }

    ASSERT(DeviceInstanceHandle);

    IdNameString[0] = L"HardwareID";
    IdNameString[1] = L"CompatibleIDs";

    for (ix = 0; ix < 2; ix++)
    {
        ValueInfo = NULL;

        Status = IopGetRegistryValue(DeviceInstanceHandle,
                                     IdNameString[ix],
                                     &ValueInfo);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("PiCriticalOpenCriticalDeviceKey: Status - %X\n", Status);
        }
        else
        {
            ASSERT(ValueInfo);

            if (ValueInfo->Type == REG_MULTI_SZ)
            {
                IdBuffer = (PWCHAR)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);

                CddNameString.Buffer = IdBuffer;
                CddNameString.Length = ValueInfo->DataLength;
                CddNameString.MaximumLength = CddNameString.Length;

                Status = IopReplaceSeparatorWithPound(&CddNameString,
                                                      &CddNameString);
                ASSERT(NT_SUCCESS(Status));

                Status = PiCriticalOpenFirstMatchingSubKey(IdBuffer,
                                                           DatabaseRootHandle,
                                                           KEY_READ,
                                                           PiCriticalCallbackVerifyCriticalEntry,
                                                           CriticalDeviceEntryHandle);

                ExFreePoolWithTag(ValueInfo, 'uspP');

                if (!NT_SUCCESS(Status))
                {
                    DPRINT("PiCriticalOpenCriticalDeviceKey: Status - %X\n", Status);
                    continue;
                }
                else
                {
                    ASSERT(*CriticalDeviceEntryHandle);
                    ZwClose(DeviceInstanceHandle);
                    goto Exit;
                }
            }

            Status = STATUS_UNSUCCESSFUL;
            ExFreePoolWithTag(ValueInfo, 'uspP');
        }
    }

    if (DeviceInstanceHandle)
    {
        ZwClose(DeviceInstanceHandle);
    }

Exit:

    if (!Handle && DatabaseRootHandle)
    {
        ZwClose(DatabaseRootHandle);
    }

    return Status;
}

NTSTATUS
NTAPI
PpCriticalProcessCriticalDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo = NULL;
    HANDLE CriticalDeviceEntryHandle = NULL;
    HANDLE DeviceInstanceHandle = NULL;
    UNICODE_STRING ValueName;
    ULONG ConfigFlags = 0;
    NTSTATUS Status;
    NTSTATUS status;

    PAGED_CODE();
    DPRINT("PpCriticalProcessCriticalDevice: DeviceNode - %p\n", DeviceNode);

    if (!DeviceNode)
    {
        DPRINT("PpCriticalProcessCriticalDevice: DeviceNode - NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (!PiCriticalDeviceDatabaseEnabled)
    {
        DPRINT("PpCriticalProcessCriticalDevice: PiCriticalDeviceDatabaseEnabled - FALSE\n");
        return STATUS_NOT_SUPPORTED;
    }

    Status = PiCriticalOpenCriticalDeviceKey(DeviceNode,
                                             NULL,
                                             &CriticalDeviceEntryHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PpCriticalProcessCriticalDevice: Status - %X\n", Status);
        ASSERT(CriticalDeviceEntryHandle == NULL);
        goto Exit;
    }

    ASSERT(CriticalDeviceEntryHandle != NULL);

    Status = PnpDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject,
                                             &DeviceInstanceHandle,
                                             KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PpCriticalProcessCriticalDevice: Status - %X\n", Status);
        goto Exit;
    }

    ASSERT(DeviceInstanceHandle != NULL);

    Status = PiCriticalCopyCriticalDeviceProperties(DeviceInstanceHandle,
                                                    CriticalDeviceEntryHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PpCriticalProcessCriticalDevice: Status - %X\n", Status);
        goto Exit;
    }

    DPRINT("PpCriticalProcessCriticalDevice: FIXME PiCriticalPreInstallDevice\n");

    status = IopGetRegistryValue(DeviceInstanceHandle,
                                 L"ConfigFlags",
                                 &ValueInfo);
    if (NT_SUCCESS(status))
    {
        ASSERT(ValueInfo);

        if (ValueInfo->Type == REG_DWORD && 
            ValueInfo->DataLength == sizeof(ULONG))
        {
            ConfigFlags = *(PULONG)((ULONG_PTR)ValueInfo +
                                    ValueInfo->DataOffset);
        }

        ExFreePoolWithTag(ValueInfo, 'uspP');
    }

    DPRINT("PpCriticalProcessCriticalDevice: ConfigFlags - %X\n", ConfigFlags);
    ConfigFlags = (ConfigFlags & ~(0x20 | 0x40)) | 0x400; // ??

    RtlInitUnicodeString(&ValueName, L"ConfigFlags");

    ZwSetValueKey(DeviceInstanceHandle,
                  &ValueName,
                  0,
                  REG_DWORD,
                  &ConfigFlags,
                  sizeof(ConfigFlags));

    if (DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM))
    {
        ASSERT(DeviceNode->Flags & DNF_HAS_PROBLEM);

        ASSERT(DeviceNode->Problem == CM_PROB_NOT_CONFIGURED ||
               DeviceNode->Problem == CM_PROB_REINSTALL ||
               DeviceNode->Problem == CM_PROB_FAILED_INSTALL);
    }

    PipClearDevNodeProblem(DeviceNode);

Exit:

    if (CriticalDeviceEntryHandle)
    {
        ZwClose(CriticalDeviceEntryHandle);
    }

    if (DeviceInstanceHandle)
    {
        ZwClose(DeviceInstanceHandle);
    }

    return Status;
}

NTSTATUS
NTAPI
PiProcessNewDeviceNode(
    _In_ PDEVICE_NODE DeviceNode)
{
    PKEY_VALUE_FULL_INFORMATION KeyInfo;
    UNICODE_STRING ValueName;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT DupeDeviceObject;
    DEVICE_CAPABILITIES DeviceCapabilities;
    PWCHAR DeviceID;
    PWCHAR FullDeviceID;
    PWCHAR LocationInformation;
    PWCHAR Description;
    PWCHAR InstanceID;
    ULONG InstanceIdSize;
    PWCHAR HardwareIDs;
    ULONG HardwareIDsSize;
    PWCHAR CompatibleIDs;
    ULONG CompatibleIDsSize;
    HANDLE Handle;
    HANDLE KeyHandle;
    ULONG Disposition = 0;
    ULONG Problem;
    ULONG ConfigFlags;
    NTSTATUS status;
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN GloballyUnique;
    BOOLEAN HasProblem;
    BOOLEAN CreatedNewKey = FALSE;

    PAGED_CODE();
    DPRINT("PiProcessNewDeviceNode: DeviceNode - %p\n", DeviceNode);

    DeviceObject = DeviceNode->PhysicalDeviceObject;

    Status = PpQueryDeviceID(DeviceNode, &FullDeviceID, &DeviceID);

    if (Status == STATUS_PNP_INVALID_ID)
    {
        DPRINT("PiProcessNewDeviceNode: STATUS_PNP_INVALID_ID\n");
        Status = STATUS_UNSUCCESSFUL;
    }

    DeviceNode->UserFlags &= ~DNUF_DONT_SHOW_IN_UI;
    GloballyUnique = FALSE;

    Status = PpIrpQueryCapabilities(DeviceObject, &DeviceCapabilities);

    if (NT_SUCCESS(Status))
    {
        if (DeviceCapabilities.NoDisplayInUI)
        {
            DeviceNode->UserFlags = DeviceNode->UserFlags | DNUF_DONT_SHOW_IN_UI;
        }

        if (DeviceCapabilities.UniqueID)
        {
            GloballyUnique = TRUE;
        }
    }

    DPRINT("PiProcessNewDeviceNode: FIXME PpProfileProcessDockDeviceCapability\n");

    PpIrpQueryDeviceText(DeviceNode->PhysicalDeviceObject,
                         DeviceTextDescription,
                         PsDefaultSystemLocaleId,
                         &Description);

    PpIrpQueryDeviceText(DeviceNode->PhysicalDeviceObject,
                         DeviceTextLocationInformation,
                         PsDefaultSystemLocaleId,
                         &LocationInformation);

    Status = PpQueryID(DeviceNode,
                       BusQueryInstanceID,
                       &InstanceID,
                       &InstanceIdSize);

    ASSERT(Status != STATUS_NOT_SUPPORTED || !GloballyUnique);

    if (GloballyUnique)
    {
        DPRINT("PiProcessNewDeviceNode: GloballyUnique\n");

        if (Status == STATUS_NOT_SUPPORTED)
        {
            PipSetDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA);
            DeviceNode->Parent->Flags |= DNF_CHILD_WITH_INVALID_ID;

            DPRINT("PiProcessNewDeviceNode: FIXME PpSetInvalidIDEvent\n");
        }
    }
    else if ((!(DeviceNode->Flags & DNF_HAS_PROBLEM) ||
              (DeviceNode->Problem != CM_PROB_INVALID_DATA)) &&
             (DeviceNode->Parent != IopRootDeviceNode))
    {
        ValueName.Buffer = NULL;

        Status = PipMakeGloballyUniqueId(DeviceObject,
                                         InstanceID,
                                         &ValueName.Buffer);
        if (InstanceID)
        {
            ExFreePool(InstanceID);
        }

        InstanceID = ValueName.Buffer;
        DPRINT("PiProcessNewDeviceNode: InstanceID - %S\n", InstanceID);

        if (ValueName.Buffer == NULL)
        {
            InstanceIdSize = 0;
            ASSERT(!NT_SUCCESS(Status));
        }
        else
        {
            InstanceIdSize = (wcslen(ValueName.Buffer) + 1) * sizeof(WCHAR);
        }
    }

    while (TRUE)
    {
        if (!NT_SUCCESS(Status))
        {
            DPRINT("PiProcessNewDeviceNode: Status - %X\n", Status);

            if (!(DeviceNode->Flags & DNF_HAS_PROBLEM) ||
                DeviceNode->Problem != CM_PROB_INVALID_DATA)
            {
                if (Status == STATUS_INSUFFICIENT_RESOURCES)
                {
                    PipSetDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY);
                }
                else
                {
                    PipSetDevNodeProblem(DeviceNode, CM_PROB_REGISTRY);
                }
            }
        }

        status = PiBuildDeviceNodeInstancePath(DeviceNode,
                                               FullDeviceID,
                                               DeviceID,
                                               InstanceID);
        if (NT_SUCCESS(status))
        {
            status = PiCreateDeviceInstanceKey(DeviceNode,
                                               &KeyHandle,
                                               &Disposition);
        }
        else
        {
            DPRINT("PiProcessNewDeviceNode: status - %X\n", status);
            Status = status;
        }

        PpMarkDeviceStackStartPending(DeviceObject, TRUE);
        PipSetDevNodeState(DeviceNode, DeviceNodeInitialized, NULL);

        if (DeviceNode->Flags & DNF_HAS_PROBLEM &&
            (DeviceNode->Problem == CM_PROB_INVALID_DATA ||
             DeviceNode->Problem == CM_PROB_OUT_OF_MEMORY ||
             DeviceNode->Problem == CM_PROB_REGISTRY))
        {
            break;
        }

        if (Disposition == REG_CREATED_NEW_KEY)
        {
            KeEnterCriticalRegion();
            ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

            if (KeyHandle)
            {
                if (Description)
                {
                    RtlInitUnicodeString(&ValueName, L"DeviceDesc");

                    DPRINT("PiProcessNewDeviceNode: Description - %S, size - %X\n",
                           Description, sizeof(WCHAR) * wcslen(Description) + sizeof(WCHAR));

                    ZwSetValueKey(KeyHandle,
                                  &ValueName,
                                  0,
                                  REG_SZ,
                                  Description,
                                  sizeof(WCHAR) * wcslen(Description) + sizeof(WCHAR));
                }
                else
                {
                    DPRINT("PiProcessNewDeviceNode: HACK!!! Description == 0. FIXME\n");

                    RtlInitUnicodeString(&ValueName, L"DeviceDesc");

                    DPRINT("PiProcessNewDeviceNode: Description - %S, size - %X\n",
                           L"Unknown device", sizeof(WCHAR) * wcslen(L"Unknown device") + sizeof(WCHAR));

                    ZwSetValueKey(KeyHandle,
                                  &ValueName,
                                  0,
                                  REG_SZ,
                                  L"Unknown device",
                                  sizeof(WCHAR) * wcslen(L"Unknown device") + sizeof(WCHAR));
                }
            }
            else
            {
                DPRINT("PiProcessNewDeviceNode: KeyHandle == 0\n");
            }

            if (Description)
            {
                ExFreePool(Description);
                Description = NULL;
            }

            ExReleaseResourceLite(&PpRegistryDeviceResource);
            KeLeaveCriticalRegion();
            break;
        }

        DupeDeviceObject = IopDeviceObjectFromDeviceInstance(&DeviceNode->InstancePath);

        if (!DupeDeviceObject)
        {
            break;
        }

        if (DupeDeviceObject == DeviceObject)
        {
            DPRINT1("PiProcessNewDeviceNode: DupeDeviceObject\n");
            ASSERT(FALSE);
            ObDereferenceObject(DupeDeviceObject);
            break;
        }

        if (!GloballyUnique)
        {
            DPRINT1("PiProcessNewDeviceNode: DupeDeviceObject\n");
            ASSERT(FALSE);
            KeBugCheckEx(PNP_DETECTED_FATAL_ERROR,
                         1,
                         (ULONG_PTR)DeviceObject,
                         (ULONG_PTR)DupeDeviceObject,
                         0);
        }

        GloballyUnique = FALSE;
        PipSetDevNodeProblem(DeviceNode, CM_PROB_DUPLICATE_DEVICE);

        DPRINT1("PiProcessNewDeviceNode: CM_PROB_DUPLICATE_DEVICE!\n");
        ASSERT(FALSE);
    }

    HasProblem = (DeviceNode->Flags & DNF_HAS_PROBLEM);

    if (!HasProblem ||
        (DeviceNode->Problem != CM_PROB_INVALID_DATA &&
         DeviceNode->Problem != CM_PROB_OUT_OF_MEMORY &&
         DeviceNode->Problem != CM_PROB_REGISTRY))
    {
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

        if (KeyHandle && LocationInformation)
        {
            RtlInitUnicodeString(&ValueName, L"LocationInformation");

            ZwSetValueKey(KeyHandle,
                          &ValueName,
                          0,
                          REG_SZ,
                          LocationInformation,
                          sizeof(WCHAR) * wcslen(LocationInformation) + sizeof(WCHAR));
        }

        if (LocationInformation)
        {
            ExFreePool(LocationInformation);
            LocationInformation = NULL;
        }

        PpSaveDeviceCapabilities(DeviceNode, &DeviceCapabilities);

        Problem = 0;
        CreatedNewKey = Disposition == REG_CREATED_NEW_KEY;

        status = IopGetRegistryValue(KeyHandle, L"ConfigFlags", &KeyInfo);

        if (NT_SUCCESS(status))
        {
            ConfigFlags = *(PULONG)((ULONG_PTR)&KeyInfo->TitleIndex +
                                    KeyInfo->DataOffset);

            if (ConfigFlags & 0x20) // ?
            {
                Problem = CM_PROB_REINSTALL;
                CreatedNewKey = TRUE;
                ExFreePoolWithTag(KeyInfo, 'uspP');
            }
            else if (ConfigFlags & 0x40) // ?
            {
                Problem = CM_PROB_FAILED_INSTALL;
                CreatedNewKey = TRUE;
                ExFreePoolWithTag(KeyInfo, 'uspP');
            }
            else
            {
                ExFreePoolWithTag(KeyInfo, 'uspP');
            }
        }
        else
        {
            DPRINT("PiProcessNewDeviceNode: status - %X\n", status);
            ConfigFlags = 0;
            Problem = CM_PROB_NOT_CONFIGURED;
            CreatedNewKey = TRUE;
        }

        DPRINT("PiProcessNewDeviceNode: CreatedNewKey - %X, Problem - %X\n",
               CreatedNewKey, Problem);

        if (Problem)
        {
            if (DeviceCapabilities. RawDeviceOK)
            {
                ConfigFlags |= 0x400; // ?
                RtlInitUnicodeString(&ValueName, L"ConfigFlags");

                ZwSetValueKey(KeyHandle,
                              &ValueName,
                              0,
                              REG_DWORD,
                              &ConfigFlags,
                              sizeof(ConfigFlags));
            }
            else
            {
                PipSetDevNodeProblem(DeviceNode, Problem);
            }
        }

        DPRINT("PiProcessNewDeviceNode: InstancePath - %wZ\n",
               &DeviceNode->InstancePath);

        status = IopMapDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject,
                                                    &DeviceNode->InstancePath);
        if (!NT_SUCCESS(status))
        {
            DPRINT("PiProcessNewDeviceNode: status - %X\n", status);
            ASSERT(NT_SUCCESS(status));
            Status = status;
        }

        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();
    }

    PpQueryID(DeviceNode,
              BusQueryHardwareIDs,
              &HardwareIDs,
              &HardwareIDsSize);

    PpQueryID(DeviceNode,
              BusQueryCompatibleIDs,
              &CompatibleIDs,
              &CompatibleIDsSize);

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    DeviceNode->Flags |= DNF_IDS_QUERIED;

    HasProblem = DeviceNode->Flags & DNF_HAS_PROBLEM;

    if (!HasProblem ||
        (DeviceNode->Problem != CM_PROB_INVALID_DATA &&
         DeviceNode->Problem != CM_PROB_OUT_OF_MEMORY &&
         DeviceNode->Problem != CM_PROB_REGISTRY))
    {
        RtlInitUnicodeString(&ValueName, L"LogConf");

        IopCreateRegistryKeyEx(&Handle,
                               KeyHandle,
                               &ValueName,
                               KEY_ALL_ACCESS,
                               REG_OPTION_NON_VOLATILE,
                               NULL);
    }

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    PiQueryResourceRequirements(DeviceNode, Handle);
    PipDumpResourceRequirementsList(DeviceNode->ResourceRequirements, 1);

    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

    DPRINT("PiProcessNewDeviceNode: FIXME IopIsRemoteBootCard()\n");

    if (KeyHandle)
    {
        if (HardwareIDs)
        {
            RtlInitUnicodeString(&ValueName, L"HardwareID");

            ZwSetValueKey(KeyHandle,
                          &ValueName,
                          0,
                          REG_MULTI_SZ,
                          HardwareIDs,
                          HardwareIDsSize);
        }
    }

    if (HardwareIDs)
    {
        ExFreePool(HardwareIDs);
        HardwareIDs = NULL;
    }

    if (KeyHandle)
    {
        if (CompatibleIDs)
        {
            RtlInitUnicodeString(&ValueName, L"CompatibleIDs");

            ZwSetValueKey(KeyHandle,
                          &ValueName,
                          0,
                          REG_MULTI_SZ,
                          CompatibleIDs,
                          CompatibleIDsSize);
        }
    }

    if (CompatibleIDs)
    {
        ExFreePool(CompatibleIDs);
        CompatibleIDs = NULL;
    }

    Status = STATUS_SUCCESS;

    DPRINT("PiProcessNewDeviceNode: FIXME IopSetupRemoteBootCard()\n");

    ExReleaseResourceLite(&PpRegistryDeviceResource);
    KeLeaveCriticalRegion();

    PpQueryBusInformation(DeviceNode);

    if (NT_SUCCESS(Status))
    {
        if (CreatedNewKey &&
            !DeviceCapabilities.HardwareDisabled &&
            (!(DeviceNode->Flags & DNF_HAS_PROBLEM) ||
             DeviceNode->Problem != CM_PROB_NEED_RESTART))
        {
            PpCriticalProcessCriticalDevice(DeviceNode);
        }

        if (DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM))
        {
            DPRINT("PiProcessNewDeviceNode: Flags - %X, Problem - %X\n",
                   DeviceNode->Flags, DeviceNode->Problem);

            ASSERT(DeviceNode->Flags & DNF_HAS_PROBLEM);

            ASSERT(DeviceNode->Problem == CM_PROB_NOT_CONFIGURED ||
                   DeviceNode->Problem == CM_PROB_REINSTALL ||
                   DeviceNode->Problem == CM_PROB_FAILED_INSTALL ||
                   DeviceNode->Problem == CM_PROB_PARTIAL_LOG_CONF ||
                   DeviceNode->Problem == CM_PROB_HARDWARE_DISABLED ||
                   DeviceNode->Problem == CM_PROB_NEED_RESTART ||
                   DeviceNode->Problem == CM_PROB_DUPLICATE_DEVICE ||
                   DeviceNode->Problem == CM_PROB_INVALID_DATA ||
                   DeviceNode->Problem == CM_PROB_OUT_OF_MEMORY ||
                   DeviceNode->Problem == CM_PROB_REGISTRY);
        }

        HasProblem = (DeviceNode->Flags & DNF_HAS_PROBLEM);

        if (!HasProblem ||
            (DeviceNode->Problem != CM_PROB_DISABLED &&
             DeviceNode->Problem != CM_PROB_HARDWARE_DISABLED &&
             DeviceNode->Problem != CM_PROB_NEED_RESTART &&
             DeviceNode->Problem != CM_PROB_INVALID_DATA &&
             DeviceNode->Problem != CM_PROB_OUT_OF_MEMORY &&
             DeviceNode->Problem != CM_PROB_REGISTRY))
        {
            IopIsDeviceInstanceEnabled(KeyHandle,
                                       &DeviceNode->InstancePath,
                                       TRUE);
        }
    }
    else
    {
        DPRINT("PiProcessNewDeviceNode: Status - %X\n", Status);
    }

    PiQueryAndAllocateBootResources(DeviceNode, Handle);

    HasProblem = DeviceNode->Flags & DNF_HAS_PROBLEM;

    if (!HasProblem ||
        (DeviceNode->Problem != CM_PROB_INVALID_DATA &&
         DeviceNode->Problem != CM_PROB_OUT_OF_MEMORY &&
         DeviceNode->Problem != CM_PROB_REGISTRY))
    {
        NTSTATUS status;

        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&PpRegistryDeviceResource, TRUE);

        PpSaveDeviceCapabilities(DeviceNode, &DeviceCapabilities);

        ExReleaseResourceLite(&PpRegistryDeviceResource);
        KeLeaveCriticalRegion();

        PpHotSwapUpdateRemovalPolicy(DeviceNode);

        DPRINT("PiProcessNewDeviceNode: FIXME IopNotifySetupDeviceArrival\n");

        status = PpDeviceRegistration(&DeviceNode->InstancePath,
                                      TRUE,
                                      &DeviceNode->ServiceName);

        if (NT_SUCCESS(status) &&
            (DeviceNode->Flags & DNF_HAS_PROBLEM) &&
            DeviceNode->Problem == CM_PROB_NOT_CONFIGURED)
        {
            PipClearDevNodeProblem(DeviceNode);
        }

        DPRINT("PiProcessNewDeviceNode: FIXME PpSetPlugPlayEvent\n");

        /* Report the device to the user-mode pnp manager */
        IopQueueTargetDeviceEvent(&GUID_DEVICE_ENUMERATED,
                                  &DeviceNode->InstancePath);
    }

    if (HardwareIDs)
    {
        ExFreePool(HardwareIDs);
    }
    if (CompatibleIDs)
    {
        ExFreePool(CompatibleIDs);
    }
    if (Handle)
    {
        ZwClose(Handle);
    }
    if (KeyHandle)
    {
        ZwClose(KeyHandle);
    }
    if (InstanceID)
    {
        ExFreePool(InstanceID);
    }
    if (LocationInformation)
    {
        ExFreePool(LocationInformation);
    }
    if (Description)
    {
        ExFreePool(Description);
    }
    if (FullDeviceID)
    {
        ExFreePool(FullDeviceID);
    }

    DPRINT("PiProcessNewDeviceNode: exit Status - %X\n", Status);

    return Status;
}

NTSTATUS
NTAPI 
IopUnloadAttachedDriver(
    _In_ PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING RegServicesName = RTL_CONSTANT_STRING(IO_REG_KEY_SERVICES);
    UNICODE_STRING FullServiceName;
    PUNICODE_STRING ServiceKeyName;
    PWSTR Buffer;
    ULONG Length;
    NTSTATUS Status;

    PAGED_CODE();

    ServiceKeyName = &DriverObject->DriverExtension->ServiceKeyName;
    DPRINT("IopUnloadAttachedDriver: ServiceKeyName '%wZ'\n", ServiceKeyName);

    if (!DriverObject->DriverSection)
    {
        DPRINT("IopUnloadAttachedDriver: Skipping unload '%wZ'\n", ServiceKeyName);
        return STATUS_SUCCESS;
    }

    if (DriverObject->DeviceObject)
    {
        DPRINT("IopUnloadAttachedDriver: Skipping unload, DeviceObject %p\n", DriverObject->DeviceObject);
        return STATUS_SUCCESS;
    }

    Length = (ServiceKeyName->Length + RegServicesName.Length + 6);

    Buffer = ExAllocatePoolWithTag(PagedPool, Length, 'edpP');
    if (!Buffer)
    {
        DPRINT1("IopUnloadAttachedDriver: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlStringCbPrintfW(Buffer,
                       Length,
                       L"%s\\%s",
                       RegServicesName.Buffer,
                       ServiceKeyName->Buffer);

    RtlInitUnicodeString(&FullServiceName, Buffer);

    Status = IopUnloadDriver(&FullServiceName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopUnloadAttachedDriver: Error unloading '%wZ', Status %X\n", ServiceKeyName, Status);
        goto Exit;
    }

    DPRINT("IopUnloadAttachedDriver: Unloaded '%wZ'\n", ServiceKeyName);

Exit:
    ExFreePoolWithTag(FullServiceName.Buffer, 'edpP');
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
PipGetRegistryDwordWithFallback(
    _In_ PUNICODE_STRING ValueName,
    _In_ HANDLE Handle,
    _In_ HANDLE PropertiesHandle,
    _Out_ PULONG OutValue)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    HANDLE KeyHandle[3];
    ULONG Count = 0;
    ULONG ix;
    NTSTATUS Status;
    BOOLEAN Result = FALSE;

    if (Handle)
    {
        KeyHandle[Count++] = Handle;
    }

    if (PropertiesHandle)
    {
        KeyHandle[Count++] = PropertiesHandle;
    }

    KeyHandle[Count] = NULL;

    for (ix = 0;
         ix < Count && Result == FALSE;
         ix++)
    {
        ValueInfo = NULL;

        Status = IopGetRegistryValue(KeyHandle[ix],
                                     ValueName->Buffer,
                                     &ValueInfo);

        if (NT_SUCCESS(Status) && ValueInfo->Type == REG_DWORD)
        {
            *OutValue = *(PULONG)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset);
            Result = TRUE;
        }

        if (ValueInfo)
        {
            ExFreePoolWithTag(ValueInfo, 'uspP');
        }
    }

    return Result;
}

NTSTATUS
NTAPI 
PipChangeDeviceObjectFromRegistryProperties(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ HANDLE PropertiesHandle,
    _In_ HANDLE KeyHandle,
    _In_ BOOLEAN IsUsePdosSettings)
{
    UNICODE_STRING ValueName;
    PDEVICE_NODE DeviceNode;
    PDEVICE_OBJECT Device;
    ULONG Exclusive;
    ULONG DeviceType;
    ULONG DeviceCharacteristics;
    ULONG Characteristics;
    BOOLEAN OverideDeviceCharacteristics;
    //BOOLEAN OverideSecurity;
    BOOLEAN OverideExclusive;
    BOOLEAN OverideDeviceType;

    PAGED_CODE();
    ASSERT(PhysicalDeviceObject);

    DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
    ASSERT(DeviceNode);

    DPRINT("PipChangeDeviceObjectFromRegistryProperties: InstancePath - %wZ\n",
           &DeviceNode->InstancePath);

    RtlInitUnicodeString(&ValueName, L"DeviceType");
    OverideDeviceType = PipGetRegistryDwordWithFallback(&ValueName,
                                                        KeyHandle,
                                                        PropertiesHandle,
                                                        &DeviceType);
    if (!OverideDeviceType)
    {
        DeviceType = 0;
    }

    RtlInitUnicodeString(&ValueName, L"Exclusive");
    OverideExclusive = PipGetRegistryDwordWithFallback(&ValueName,
                                                       KeyHandle,
                                                       PropertiesHandle,
                                                       &Exclusive);
    if (!OverideExclusive)
    {
        Exclusive = 0;
    }

    RtlInitUnicodeString(&ValueName, L"DeviceCharacteristics");
    OverideDeviceCharacteristics = PipGetRegistryDwordWithFallback(&ValueName,
                                                                   KeyHandle,
                                                                   PropertiesHandle,
                                                                   &DeviceCharacteristics);
    if (!OverideDeviceCharacteristics)
    {
        DeviceCharacteristics = 0;
    }

    Device = PhysicalDeviceObject;

    if (IsUsePdosSettings || Device->AttachedDevice == NULL)
    {
        DPRINT("PipChangeDeviceObjectFromRegistryProperties: IsUsePdosSettings - %X\n",
               IsUsePdosSettings);
    }
    else
    {
        DPRINT("PipChangeDeviceObjectFromRegistryProperties: Ignoring PDO's settings\n");
        Device = Device->AttachedDevice;
    }

    Characteristics = 0;

    for (; Device; Device = Device->AttachedDevice)
    {
        Characteristics |= Device->Characteristics;
    }

    DeviceCharacteristics |= Characteristics;
    DeviceCharacteristics &= (FILE_REMOVABLE_MEDIA |
                              FILE_READ_ONLY_DEVICE |
                              FILE_FLOPPY_DISKETTE |
                              FILE_WRITE_ONCE_MEDIA |
                              FILE_DEVICE_SECURE_OPEN);

    DPRINT("PipChangeDeviceObjectFromRegistryProperties: FIXME 'Security'\n");

    if (!OverideDeviceType &&
        !OverideDeviceCharacteristics &&
        !OverideExclusive)// && !SecurityDesc)
    {
        DPRINT("PipChangeDeviceObjectFromRegistryProperties: No property changes\n");
    }
    else
    {
        if (OverideDeviceType)
        {
            DPRINT("PipChangeDeviceObjectFromRegistryProperties: DeviceType - %X\n",
                   DeviceType);
        }
        if (OverideDeviceCharacteristics)
        {
            DPRINT("PipChangeDeviceObjectFromRegistryProperties: DeviceCharacteristics - %X\n",
                   DeviceCharacteristics);
        }
        if (OverideExclusive)
        {
            DPRINT("PipChangeDeviceObjectFromRegistryProperties: Exclusive - %X\n",
                   Exclusive);
        }
    }

    if (OverideDeviceType)
    {
        PhysicalDeviceObject->DeviceType = DeviceType;
    }
    if (OverideExclusive && Exclusive)
    {
        PhysicalDeviceObject->Flags |= DOE_REMOVE_PROCESSED;
    }

    PhysicalDeviceObject->Characteristics |= DeviceCharacteristics;
    PhysicalDeviceObject->Characteristics &= ~(FILE_REMOVABLE_MEDIA |
                                               FILE_READ_ONLY_DEVICE |
                                               FILE_FLOPPY_DISKETTE |
                                               FILE_WRITE_ONCE_MEDIA |
                                               FILE_DEVICE_SECURE_OPEN);

    for (Device = PhysicalDeviceObject->AttachedDevice;
         Device;
         Device = Device->AttachedDevice)
    {
        Device->Characteristics |= DeviceCharacteristics;
    }

    return STATUS_SUCCESS;
}

PDRIVER_OBJECT
NTAPI
IopReferenceDriverObjectByName(
    _In_ PUNICODE_STRING Name)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    PDRIVER_OBJECT DriverObject;
    HANDLE Handle;
    PVOID Object;
    NTSTATUS Status;

    if (!Name->Length)
    {
        DPRINT("IopReferenceDriverObjectByName: Name->Length == 0\n");
        return NULL;
    }

    DPRINT("IopReferenceDriverObjectByName: Name - %wZ\n", Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               Name,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ObOpenObjectByName(&ObjectAttributes,
                                IoDriverObjectType,
                                KernelMode,
                                NULL,
                                FILE_READ_ATTRIBUTES,
                                NULL,
                                &Handle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopReferenceDriverObjectByName: Status - %X\n", Status);
        return NULL;
    }

    Status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       IoDriverObjectType,
                                       KernelMode,
                                       &Object,
                                       NULL);
    NtClose(Handle);
         
    if (NT_SUCCESS(Status))
    {
        DriverObject = (PDRIVER_OBJECT)Object;
    }
    else
    {
        DPRINT("IopReferenceDriverObjectByName: Status - %X\n", Status);
        DriverObject = NULL;
    }

    return DriverObject;
}

NTSTATUS
NTAPI
PipLoadBootFilterDriver(
    _In_ HANDLE Handle,
    _In_ PUNICODE_STRING DriverName,
    _In_ ULONG GroupIndex,
    _Out_ PDRIVER_OBJECT * OutDriverObject)
{
    PDRIVER_INFORMATION DriverInfo;
    PDRIVER_OBJECT DriverObject;
    UNICODE_STRING NameString;
    PLIST_ENTRY Entry;
    NTSTATUS Status;
    NTSTATUS status;

    *OutDriverObject = NULL;
    Status = STATUS_UNSUCCESSFUL;

    DPRINT("PipLoadBootFilterDriver: (%X) '%wZ'\n", GroupIndex, DriverName);

    if (!IopGroupTable)
    {
        DPRINT1("PipLoadBootFilterDriver: STATUS_UNSUCCESSFUL\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (GroupIndex >= IopGroupIndex)
    {
        DPRINT1("PipLoadBootFilterDriver: STATUS_UNSUCCESSFUL\n");
        return STATUS_UNSUCCESSFUL;
    }

    for (Entry = IopGroupTable[GroupIndex].Flink;
         Entry != &IopGroupTable[GroupIndex];
         Entry = Entry->Flink)
    {
        DriverInfo = CONTAINING_RECORD(Entry, DRIVER_INFORMATION, Link);

        status = IopGetDriverNameFromKeyNode(DriverInfo->ServiceHandle, &NameString);
        if (!NT_SUCCESS(status))
        {
            DPRINT("PipLoadBootFilterDriver: status %X\n", status);
            continue;
        }

        if (!RtlEqualUnicodeString(DriverName, &NameString, TRUE))
        {
            ExFreePoolWithTag(NameString.Buffer, 0);
            continue;
        }

        if (DriverInfo->Processed)
        {
            if (!NT_SUCCESS(DriverInfo->Status))
                Status = DriverInfo->Status;
            else
                Status = STATUS_UNSUCCESSFUL;

            ExFreePoolWithTag(NameString.Buffer, 0);
            break;
        }

        Status = IopInitializeBuiltinDriver(&NameString,
                                            &DriverInfo->DataTableEntry->RegistryPath,
                                            DriverInfo->DataTableEntry->LdrEntry->EntryPoint,
                                            DriverInfo->DataTableEntry->LdrEntry,
                                            TRUE,
                                            &DriverObject);
        DriverInfo->Status = Status;
        DriverInfo->Processed = 1;
        DriverInfo->DriverObject = DriverObject;

        if (DriverObject)
        {
            ObReferenceObject(DriverObject);
            *OutDriverObject = DriverObject;
        }
        else
        {
            DriverInfo->Failed = 1;
        }

        ExFreePoolWithTag(NameString.Buffer, 0);
        break;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PipLoadBootFilterDriver: Status %X\n", Status);
    }

    return Status;
}

NTSTATUS
NTAPI
PipCallDriverAddDeviceQueryRoutine(
    _In_ PWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID ValueData,
    _In_ ULONG ValueLength,
    _In_ PVOID Context,
    _In_ PVOID ServiceType)
{
    PDRIVER_ADD_DEVICE_CONTEXT QueryContext = Context;
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    PDRIVER_ADD_DEVICE_ENTRY * DriverEntry;
    PDRIVER_ADD_DEVICE_ENTRY Entry;
    PDRIVER_OBJECT DriverObject;
    UNICODE_STRING DriverName;
    UNICODE_STRING ServiceName;
    PNP_DEVNODE_STATE NodeState;
    SERVICE_LOAD_TYPE ServiceLoadType;
    PWSTR Buffer;
    PWSTR NewBuffer;
    PWCHAR pChar;
    ULONG Problem;
    HANDLE ServicesHandle;
    HANDLE Handle = NULL;
    NTSTATUS Status;
    NTSTATUS InitStatus;
    SHORT OrderIndex;
    BOOLEAN IsNotMadeupService = FALSE;
    BOOLEAN IsAllocatedDriverName = FALSE;

    DPRINT("PipCallDriverAddDeviceQueryRoutine: '%ws', %X, %X, '%S'\n", ValueName, ValueType, ValueLength, ValueData);

    if (ValueType != REG_SZ)
    {
        DPRINT("PipCallDriverAddDeviceQueryRoutine: Invalid ValueType\n");
        return STATUS_SUCCESS;
    }

    if (ValueLength <= sizeof(WCHAR))
    {
        DPRINT("PipCallDriverAddDeviceQueryRoutine: ValueLength <= sizeof(WCHAR)\n");
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&ServiceName, (PCWSTR)ValueData);
    Buffer = ServiceName.Buffer;

    for (pChar = L"\\Driver\\";
         *pChar != UNICODE_NULL;
         pChar++, Buffer++)
    {
        if (*pChar != *Buffer)
        {
            /* Not madeup service */
            IsNotMadeupService = TRUE;
            break;
        }
    }

    if (IsNotMadeupService)
    {
        PUNICODE_STRING serviceName;

        serviceName = &QueryContext->DeviceNode->ServiceName;
        if (!serviceName->Length)
        {
            NewBuffer = ExAllocatePoolWithTag(NonPagedPool, ServiceName.MaximumLength, 'nepP');
            if (!NewBuffer)
            {
                DPRINT1("PipCallDriverAddDeviceQueryRoutine: Cannot allocate memory!\n");
                RtlZeroMemory(serviceName, sizeof(UNICODE_STRING));
                Status = STATUS_UNSUCCESSFUL;
                goto Exit;
            }

            serviceName->Length = ServiceName.Length;
            serviceName->MaximumLength = ServiceName.MaximumLength;
            serviceName->Buffer = NewBuffer;

            RtlCopyMemory(NewBuffer, ServiceName.Buffer, ServiceName.MaximumLength);
        }

        Status = PipOpenServiceEnumKeys(&ServiceName, KEY_READ, &Handle, NULL, FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("PipCallDriverAddDeviceQueryRoutine: Status %X\n", Status);
            PipSetDevNodeProblem(QueryContext->DeviceNode, CM_PROB_REGISTRY);
            goto Exit;
        }

        Status = IopGetDriverNameFromKeyNode(Handle, &DriverName);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("PipCallDriverAddDeviceQueryRoutine: Status %X\n", Status);
            PipSetDevNodeProblem(QueryContext->DeviceNode, CM_PROB_REGISTRY);
            goto Exit;
        }

        IsAllocatedDriverName = TRUE;

        DPRINT("PipCallDriverAddDeviceQueryRoutine: Not Madeup service '%wZ'\n", &DriverName);
    }
    else
    {
        RtlInitUnicodeString(&DriverName, ServiceName.Buffer);
        DPRINT("PipCallDriverAddDeviceQueryRoutine: Madeup service '%wZ'\n", &DriverName);
    }

    DriverObject = IopReferenceDriverObjectByName(&DriverName);
    DPRINT("PipCallDriverAddDeviceQueryRoutine: DriverObject %p\n", DriverObject);

    if (DriverObject)
        goto SetupDriver;

    if (!IsNotMadeupService)
    {
        DPRINT("PipCallDriverAddDeviceQueryRoutine: No DriverObject for madeup service\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    ServiceLoadType = DisableLoad;

    Status = IopGetRegistryValue(Handle, L"Start", &ValueInfo);
    if (NT_SUCCESS(Status))
    {
        if (ValueInfo->Type == REG_DWORD &&
            ValueInfo->DataLength == sizeof(ULONG))
        {
            ServiceLoadType = *(PULONG)((ULONG_PTR)&ValueInfo->TitleIndex + ValueInfo->DataOffset);
        }

        ExFreePoolWithTag(ValueInfo, 'uspP');
    }

    if (ServiceType == ULongToPtr(DeviceService) || PnPBootDriversInitialized)
    {
        if (!QueryContext->EnableLoadDriver)
        {
            DPRINT("PipCallDriverAddDeviceQueryRoutine: Not allowed to load drivers yet\n");
            Status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }

        if (ServiceLoadType > *QueryContext->DriverLoadType)
        {
            if (ServiceLoadType == DisableLoad &&
                !(QueryContext->DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM)))
            {
                PipSetDevNodeProblem(QueryContext->DeviceNode, CM_PROB_DISABLED_SERVICE);
            }

            DPRINT1("PipCallDriverAddDeviceQueryRoutine: Service is disabled or not at right time to load it\n");
            ASSERT(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }

        Status = PipOpenServiceEnumKeys(&ServiceName, KEY_READ, &ServicesHandle, NULL, FALSE);

        if (NT_SUCCESS(Status))
        {
            Status = IopLoadDriver(ServicesHandle,
                                   FALSE,
                                   ServiceType != ULongToPtr(DeviceService),
                                   &InitStatus);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("PipCallDriverAddDeviceQueryRoutine: IopLoadDriver('%wZ') Status %X\n", &ServiceName, Status);
                //ASSERT(FALSE);

                if (Status == STATUS_FAILED_DRIVER_ENTRY)
                {
                    if (InitStatus == STATUS_INSUFFICIENT_RESOURCES)
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else if (Status != STATUS_INSUFFICIENT_RESOURCES  &&
                         Status != STATUS_PLUGPLAY_NO_DEVICE &&
                         Status != STATUS_DRIVER_FAILED_PRIOR_UNLOAD &&
                         Status != STATUS_DRIVER_BLOCKED &&
                         Status != STATUS_DRIVER_BLOCKED_CRITICAL)
                {
                    Status = STATUS_DRIVER_UNABLE_TO_LOAD;
                }
            }

            if (PnpSystemInit)
                IopReinitializeDrivers();
        }
        else
        {
            DPRINT1("PipCallDriverAddDeviceQueryRoutine: Status %X\n", Status);
            ASSERT(FALSE);

            if (Status != STATUS_INSUFFICIENT_RESOURCES)
                Status = STATUS_ILL_FORMED_SERVICE_ENTRY;
        }

        DriverObject = IopReferenceDriverObjectByName(&DriverName);
        if (DriverObject)
            goto SetupDriver;

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PipCallDriverAddDeviceQueryRoutine: Status %X\n", Status);
            ASSERT(FALSE);

            if (/*(PiUserModeRunning == FALSE) && */ 
                Status != STATUS_DRIVER_BLOCKED_CRITICAL &&
                Status != STATUS_DRIVER_BLOCKED)
            {
                goto Exit;
            }
        }
        else
        {
            ASSERT(InitSafeBootMode);
            Status = STATUS_NOT_SAFE_MODE_DRIVER;
        }
    }
    else
    {
        OrderIndex = PpInitGetGroupOrderIndex(Handle);
        DPRINT("PipCallDriverAddDeviceQueryRoutine: OrderIndex %X\n", OrderIndex);

        Status = PipLoadBootFilterDriver(Handle, &DriverName, OrderIndex, &DriverObject);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("PipCallDriverAddDeviceQueryRoutine: Status %X\n", Status);
            ASSERT(FALSE);

            if (Status != STATUS_DRIVER_BLOCKED &&
                Status != STATUS_DRIVER_BLOCKED_CRITICAL)
            {
                goto Exit;
            }
        }
        else
        {
            PDRIVER_OBJECT tempDrvObj;

            ASSERT(DriverObject);

            tempDrvObj = IopReferenceDriverObjectByName(&DriverName);
            ASSERT(tempDrvObj == DriverObject);
        }
    }

    if (!DriverObject)
    {
        ASSERT(!NT_SUCCESS(Status));

        DPRINT("PipCallDriverAddDeviceQueryRoutine: No DriverObject. Status %X\n", Status);

        if (QueryContext->DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM))
        {
            DPRINT1("PipCallDriverAddDeviceQueryRoutine: No DriverObject. Status %X\n", Status);
            ASSERT(FALSE);
            goto Exit;
        }

        switch (Status)
        {
            case STATUS_FAILED_DRIVER_ENTRY:
                Problem = CM_PROB_FAILED_DRIVER_ENTRY;
                break;

            case STATUS_INSUFFICIENT_RESOURCES:
                Problem = CM_PROB_OUT_OF_MEMORY;
                break;

            case STATUS_ILL_FORMED_SERVICE_ENTRY:
                Problem = CM_PROB_DRIVER_SERVICE_KEY_INVALID;
                break;

            case STATUS_PLUGPLAY_NO_DEVICE:
                Problem = CM_PROB_LEGACY_SERVICE_NO_DEVICES;
                break;

            case STATUS_DRIVER_UNABLE_TO_LOAD:
                Problem = CM_PROB_DRIVER_FAILED_LOAD;
                break;

            case STATUS_DRIVER_BLOCKED_CRITICAL:
                Problem = CM_PROB_DRIVER_BLOCKED;
                QueryContext->DeviceNode->Flags |= DNF_DRIVER_BLOCKED;
                break;

            case STATUS_DRIVER_BLOCKED:
                Status = STATUS_SUCCESS;
                Problem = 0;
                QueryContext->DeviceNode->Flags |= DNF_DRIVER_BLOCKED;
                break;

            case STATUS_DRIVER_FAILED_PRIOR_UNLOAD:
                Problem = CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD;
                break;

            default:
                DPRINT1("PipCallDriverAddDeviceQueryRoutine: Status %X\n", Status);
                ASSERT(FALSE);
                Problem = CM_PROB_FAILED_ADD;
                break;
        }

        if (Problem != 0)
            PipSetDevNodeProblem(QueryContext->DeviceNode, Problem);

      #if DBG
        QueryContext->DeviceNode->DebugStatus = Status;
      #endif

        goto Exit;
    }

SetupDriver:

    if (!(DriverObject->Flags & DRVO_INITIALIZED))
    {
        ObDereferenceObject(DriverObject);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    DPRINT("PipCallDriverAddDeviceQueryRoutine: DriverObject %p\n", DriverObject);

    if (IopIsLegacyDriver(DriverObject))
    {
        DPRINT("PipCallDriverAddDeviceQueryRoutine: Is a legacy driver\n");

        if (ServiceType == ULongToPtr(DeviceService))
        {
            QueryContext->DeviceNode->Flags |= DNF_LEGACY_DRIVER;
            PipSetDevNodeState(QueryContext->DeviceNode, DeviceNodeStarted, NULL);

            Status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            Status = STATUS_SUCCESS;
        }

        goto Exit;
    }

    NodeState = QueryContext->DeviceNode->State;

    if (NodeState != DeviceNodeInitialized &&
        NodeState != DeviceNodeDriversAdded)
    {
        DPRINT("PipCallDriverAddDeviceQueryRoutine: State - %X\n", NodeState);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    DriverEntry = &QueryContext->DriverLists[(ULONG)ServiceType];
    Status = STATUS_SUCCESS;

    Entry = ExAllocatePoolWithTag(PagedPool, sizeof(DRIVER_ADD_DEVICE_ENTRY), 'nepP');
    if (!Entry)
    {
        DPRINT("PipCallDriverAddDeviceQueryRoutine: Unable to allocate memory!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Entry->DriverObject = DriverObject;
    Entry->NextEntry = NULL;

    while (*DriverEntry)
        DriverEntry = &((*DriverEntry)->NextEntry);

    *DriverEntry = Entry;

Exit:

    if (Handle)
        ZwClose(Handle);

    if (IsAllocatedDriverName)
        RtlFreeUnicodeString(&DriverName);

    return Status;
}

NTSTATUS
NTAPI
PipCallDriverAddDevice(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN IsLoadDriver,
    _In_ SERVICE_LOAD_TYPE * DriverLoadType)
{
    UNICODE_STRING EnumKeyName = RTL_CONSTANT_STRING(IO_REG_KEY_ENUM);
    UNICODE_STRING ControlClassName = RTL_CONSTANT_STRING(IO_REG_KEY_CONTROLCLASS);
    UNICODE_STRING PropertiesString = RTL_CONSTANT_STRING(L"Properties");
    UNICODE_STRING ClassGuidString;
    UNICODE_STRING DeviceDescString;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    PKEY_VALUE_FULL_INFORMATION KeyInfo = NULL;
    DRIVER_ADD_DEVICE_CONTEXT QueryContext;
    PDRIVER_ADD_DEVICE_ENTRY Entry;
    PIP_DRIVER_TYPE DriverType;
    INTERFACE_TYPE * InterfaceType;
    PDEVICE_OBJECT PDO;
    PDEVICE_OBJECT TopLowFIO = NULL;
    PDEVICE_OBJECT FDO;
    PDEVICE_OBJECT UpperDO;
    HANDLE Handle;
    HANDLE ControlClassHandle;
    HANDLE PropertiesHandle = NULL;
    HANDLE KeyHandle;
    HANDLE ClassGuidHandle = NULL;
    PULONG BusNumber;
    ULONG ix;
    NTSTATUS Status;
    USHORT Length;
    BOOLEAN DeviceRaw = TRUE;
    BOOLEAN IsUsePdosSettings;

    PAGED_CODE();
    DPRINT("PipCallDriverAddDevice: [%p] X, %X, %p\n", DeviceNode, DeviceNode->Flags, IsLoadDriver, *DriverLoadType);

    PDO = DeviceNode->PhysicalDeviceObject;

    if (PDO->Flags & DO_DEVICE_INITIALIZING)
        DPRINT("PipCallDriverAddDevice: DO_DEVICE_INITIALIZING is set!\n");

    if (!IsLoadDriver)
        DPRINT("PipCallDriverAddDevice: Won't load driver\n");

    DPRINT("PipCallDriverAddDevice: Instance '%wZ'\n", &DeviceNode->InstancePath);

    Status = IopOpenRegistryKeyEx(&Handle, NULL, &EnumKeyName, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
        return Status;
    }

    Status = IopOpenRegistryKeyEx(&KeyHandle, Handle, &DeviceNode->InstancePath, KEY_READ);
    ZwClose(Handle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
        return Status;
    }

    Status = IopGetRegistryValue(KeyHandle, L"ClassGUID", &KeyInfo);
    if (NT_SUCCESS(Status))
    {
        if (KeyInfo->Type == REG_SZ && KeyInfo->DataLength)
        {
            PnpRegSzToString((PWCHAR)((ULONG_PTR)KeyInfo + KeyInfo->DataOffset),
                             KeyInfo->DataLength,
                             &Length);

            ClassGuidString.Length = Length;
            ClassGuidString.MaximumLength = (USHORT)KeyInfo->DataLength;
            ClassGuidString.Buffer = (PWSTR)((ULONG_PTR)KeyInfo + KeyInfo->DataOffset);

            DPRINT("PipCallDriverAddDevice: ClassGuid '%wZ'\n", &ClassGuidString);

            DPRINT("PipCallDriverAddDevice: FIXME IopSafebootDriverLoad()\n");
            if (InitSafeBootMode /*&& !IopSafebootDriverLoad(&ClassGuidString)*/)
            {
                PKEY_VALUE_FULL_INFORMATION keyinfo = NULL;

                DPRINT1("SAFEBOOT: skipping '%wZ'\n", &ClassGuidString);
                ASSERT(FALSE); // IoDbgBreakPointEx();

                Status = IopGetRegistryValue(KeyHandle, L"DeviceDesc", &keyinfo);

                if (!NT_SUCCESS(Status))
                {
                    IopBootLog(&ClassGuidString, FALSE);
                }
                else
                {
                    RtlInitUnicodeString(&DeviceDescString, (PCWSTR)((ULONG_PTR)keyinfo + keyinfo->DataOffset));

                    IopBootLog(&DeviceDescString, FALSE);
                }

                ZwClose(KeyHandle);
                return STATUS_UNSUCCESSFUL;
            }

            Status = IopOpenRegistryKeyEx(&ControlClassHandle, NULL, &ControlClassName, KEY_READ);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
            }
            else
            {
                Status = IopOpenRegistryKeyEx(&ClassGuidHandle, ControlClassHandle, &ClassGuidString, KEY_READ);
                if (!NT_SUCCESS(Status))
                    DPRINT("PipCallDriverAddDevice: Status %X\n", Status);

                ZwClose(ControlClassHandle);
            }

            if (ClassGuidHandle)
            {
                Status = IopOpenRegistryKeyEx(&PropertiesHandle, ClassGuidHandle, &PropertiesString, KEY_READ);
                if (!NT_SUCCESS(Status))
                    DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
            }
        }

        ExFreePoolWithTag(KeyInfo, 'uspP');
        KeyInfo = NULL;
    }

    RtlZeroMemory(&QueryContext, sizeof(QueryContext));

    QueryContext.EnableLoadDriver = IsLoadDriver;
    QueryContext.DriverLoadType = DriverLoadType;

    RtlZeroMemory(QueryTable, sizeof(QueryTable));

    QueryTable[0].QueryRoutine = PipCallDriverAddDeviceQueryRoutine;
    QueryTable[0].Name = L"LowerFilters";
    QueryTable[0].EntryContext = ULongToPtr(LowerDeviceFilters);

    QueryContext.DeviceNode = DeviceNode;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE, (PCWSTR)KeyHandle, QueryTable, &QueryContext, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
    }
    else
    {
        if (ClassGuidHandle)
        {
            QueryTable[0].QueryRoutine = PipCallDriverAddDeviceQueryRoutine;
            QueryTable[0].Name = L"LowerFilters";
            QueryTable[0].EntryContext = ULongToPtr(LowerClassFilters);

            Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE, (PWSTR)ClassGuidHandle, QueryTable, &QueryContext, NULL);
            if (!NT_SUCCESS(Status))
                DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
        }

        if (NT_SUCCESS(Status))
        {
            QueryTable[0].QueryRoutine = PipCallDriverAddDeviceQueryRoutine;
            QueryTable[0].Flags = 4;
            QueryTable[0].Name = L"Service";
            QueryTable[0].EntryContext = ULongToPtr(DeviceService);

            Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE, (PCWSTR)KeyHandle, QueryTable, &QueryContext, NULL);
            if (!NT_SUCCESS(Status))
                DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
        }
    }

    if (DeviceNode->Flags & DNF_LEGACY_DRIVER)
    {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    if (!NT_SUCCESS(Status))
    {
        DEVICE_CAPABILITIES_FLAGS CapsFlags;

        DPRINT("PipCallDriverAddDevice: Status %X\n", Status);

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            goto Exit;

        CapsFlags.AsULONG = DeviceNode->CapabilityFlags;

        if (CapsFlags.RawDeviceOK)
        {
            PipClearDevNodeProblem(DeviceNode);
            DeviceRaw = TRUE;
            IsUsePdosSettings = TRUE;
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }
    }
    else
    {
        ASSERT(QueryContext.DriverLists[DeviceService] != NULL);

        if (QueryContext.DriverLists[DeviceService]->NextEntry)
        {
            DPRINT("PipCallDriverAddDevice: Not one service!\n");
            PipSetDevNodeProblem(DeviceNode, CM_PROB_REGISTRY);
            Status = STATUS_UNSUCCESSFUL;
            goto Exit;
        }

        IsUsePdosSettings = FALSE;
    }

    RtlZeroMemory(QueryTable, sizeof(QueryTable));

    QueryTable[0].QueryRoutine = PipCallDriverAddDeviceQueryRoutine;
    QueryTable[0].Name = L"UpperFilters";
    QueryTable[0].EntryContext = ULongToPtr(UpperDeviceFilters);

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE, (PCWSTR)KeyHandle, QueryTable, &QueryContext, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
        goto Exit;
    }

    if (ClassGuidHandle)
    {
        QueryTable[0].QueryRoutine = PipCallDriverAddDeviceQueryRoutine;
        QueryTable[0].Name = L"UpperFilters";
        QueryTable[0].EntryContext = ULongToPtr(UpperClassFilters);

        Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE, (PCWSTR)ClassGuidHandle, QueryTable, &QueryContext, NULL);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
        goto Exit;
    }

    ASSERT(!(DeviceNode->Flags & DNF_LEGACY_DRIVER));
    ASSERT((QueryContext.DriverLists[DeviceService] != NULL) || (DeviceRaw));

    FDO = NULL;
    TopLowFIO = NULL;

    for (DriverType = LowerDeviceFilters; DriverType < PipMaxServiceType; DriverType++)
    {
        DPRINT("PipCallDriverAddDevice: DriverType %X\n", DriverType);

        if (DriverType == DeviceService)
        {
            TopLowFIO = IoGetAttachedDeviceReference(PDO);

            if (DeviceRaw)
            {
                if (QueryContext.DriverLists[DeviceService])
                    ASSERT(!QueryContext.DriverLists[DeviceService]->NextEntry);
                else
                    PipSetDevNodeState(DeviceNode, DeviceNodeDriversAdded, NULL);
            }
            else
            {
                ASSERT(QueryContext.DriverLists[DeviceService]);
                ASSERT(!QueryContext.DriverLists[DeviceService]->NextEntry);
            }
        }

        for (Entry = QueryContext.DriverLists[DriverType]; Entry; Entry = Entry->NextEntry)
        {
            DPRINT("PipCallDriverAddDevice: Adding driver %p\n", Entry->DriverObject);

            ASSERT(Entry->DriverObject);
            ASSERT(Entry->DriverObject->DriverExtension);
            ASSERT(Entry->DriverObject->DriverExtension->AddDevice);

            //DPRINT("PipCallDriverAddDevice: FIXME PpvUtilCallAddDevice()\n");

            Status = Entry->DriverObject->DriverExtension->AddDevice(Entry->DriverObject, PDO);

            DPRINT("PipCallDriverAddDevice: Status %X\n", Status);

            if (!NT_SUCCESS(Status))
            {
                if (DriverType == DeviceService)
                {
                    DPRINT("PipCallDriverAddDevice: FIXME IovUtilMarkStack()\n");

                    PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_FAILED_ADD);
                    Status = STATUS_PNP_RESTART_ENUMERATION;
                    goto Exit;
                }
            }
            else
            {
                if (DriverType == DeviceService)
                {
                    FDO = TopLowFIO->AttachedDevice;
                    ASSERT(FDO);
                }

                PipSetDevNodeState(DeviceNode, DeviceNodeDriversAdded, NULL);
            }

            UpperDO = IoGetAttachedDeviceReference(PDO);

            if (UpperDO->Flags & DO_DEVICE_INITIALIZING)
                DPRINT("PipCallDriverAddDevice: DO_DEVICE_INITIALIZING!\n");

            ObDereferenceObject(UpperDO);
        }
    }

    DPRINT("PipCallDriverAddDevice: FIXME IovUtilMarkStack()\n");

    Status = PipChangeDeviceObjectFromRegistryProperties(PDO, PropertiesHandle, KeyHandle, IsUsePdosSettings);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipCallDriverAddDevice: Status %X\n", Status);

        PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_SETPROPERTIES_FAILED);
        Status = STATUS_PNP_RESTART_ENUMERATION;
        goto Exit;
    }

    BusNumber = &DeviceNode->BusNumber;
    InterfaceType = &DeviceNode->InterfaceType;

    Status = IopQueryLegacyBusInformation(PDO, 0, &DeviceNode->InterfaceType, &DeviceNode->BusNumber);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PipCallDriverAddDevice: Status %X\n", Status);
        *InterfaceType = InterfaceTypeUndefined;
        *BusNumber = 0xFFFFFFF0;
    }
    else
    {
        IopInsertLegacyBusDeviceNode(DeviceNode, *InterfaceType, *BusNumber);
    }

    Status = STATUS_SUCCESS;

    ASSERT(DeviceNode->State == DeviceNodeDriversAdded);

Exit:

    DPRINT("PipCallDriverAddDevice: DeviceNode->Flags %X\n", DeviceNode->Flags);

    for (ix = 0; ix < PipMaxServiceType; ix++)
    {
        PDRIVER_ADD_DEVICE_ENTRY Entry;
        PDRIVER_ADD_DEVICE_ENTRY PrevEntry;
        PDRIVER_ADD_DEVICE_ENTRY DriverLists;

        DriverLists = QueryContext.DriverLists[ix];

        for (Entry = DriverLists; Entry; )
        {
            ASSERT(Entry->DriverObject);

            PrevEntry = Entry;
            Entry = Entry->NextEntry;

            if (PnPBootDriversInitialized)
                IopUnloadAttachedDriver(PrevEntry->DriverObject);

            ObDereferenceObject(PrevEntry->DriverObject);
            ExFreePoolWithTag(PrevEntry, 'nepP');
        }
    }

    ZwClose(KeyHandle);

    if (ClassGuidHandle)
        ZwClose(ClassGuidHandle);

    if (PropertiesHandle)
        ZwClose(PropertiesHandle);

    if (TopLowFIO)
        ObDereferenceObject(TopLowFIO);

    DPRINT("PipCallDriverAddDevice: Returning Status %X\n", Status);

    return Status;
}

VOID
NTAPI
IopUncacheInterfaceInformation(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PPI_RESOURCE_ARBITER_ENTRY ArbiterEntry;
    PPI_RESOURCE_TRANSLATOR_ENTRY TranslatorEntry;
    PDEVICE_NODE DeviceNode;
    PLIST_ENTRY ArbiterListHead;
    PLIST_ENTRY TranslatorListHead;
    PLIST_ENTRY Entry;

    ASSERT(DeviceObject);
    DeviceNode = IopGetDeviceNode(DeviceObject);

    DPRINT("IopUncacheInterfaceInformation: DeviceObject %p, DeviceNode %p\n", DeviceObject, DeviceNode);

    ArbiterListHead = &DeviceNode->DeviceArbiterList;

    for (Entry = DeviceNode->DeviceArbiterList.Flink;
         Entry != ArbiterListHead;
        )
    {
        DPRINT("IopUncacheInterfaceInformation: Entry %p\n", Entry);

        ArbiterEntry = CONTAINING_RECORD(Entry, PI_RESOURCE_ARBITER_ENTRY, DeviceArbiterList);
        DPRINT("IopUncacheInterfaceInformation: ArbiterEntry %p\n", ArbiterEntry);

        if (ArbiterEntry->ArbiterInterface)
        {
            DPRINT("IopUncacheInterfaceInformation: ArbiterEntry->ArbiterInterface %p\n", ArbiterEntry->ArbiterInterface);
            ArbiterEntry->ArbiterInterface->InterfaceDereference(ArbiterEntry->ArbiterInterface->Context);
            ExFreePoolWithTag(ArbiterEntry->ArbiterInterface, '  pP');
        }

        Entry = Entry->Flink;
        DPRINT("IopUncacheInterfaceInformation: Entry %p\n", Entry);
        ExFreePoolWithTag(ArbiterEntry, 0);//'erpP'
    }


    TranslatorListHead = &DeviceNode->DeviceTranslatorList;
    DPRINT("IopUncacheInterfaceInformation: TranslatorListHead %p\n", TranslatorListHead);

    for (Entry = DeviceNode->DeviceTranslatorList.Flink;
         Entry != TranslatorListHead;
        )
    {
        DPRINT("IopUncacheInterfaceInformation: Entry %p\n", Entry);

        TranslatorEntry = CONTAINING_RECORD(Entry, PI_RESOURCE_TRANSLATOR_ENTRY, DeviceTranslatorList);
        DPRINT("IopUncacheInterfaceInformation: TranslatorEntry %p\n", TranslatorEntry);

        if (TranslatorEntry->TranslatorInterface)
        {
            DPRINT("IopUncacheInterfaceInformation: TranslatorEntry->TranslatorInterface %p\n", TranslatorEntry->TranslatorInterface);
            TranslatorEntry->TranslatorInterface->InterfaceDereference(TranslatorEntry->TranslatorInterface->Context);
            ExFreePoolWithTag(TranslatorEntry->TranslatorInterface, '  pP');
        }

        Entry = Entry->Flink;
        DPRINT("IopUncacheInterfaceInformation: Entry %p\n", Entry);
        ExFreePoolWithTag(TranslatorEntry, 0);//'erpP'
    }

    InitializeListHead(ArbiterListHead);
    InitializeListHead(TranslatorListHead);

    DeviceNode->NoArbiterMask = 0;
    DeviceNode->NoTranslatorMask = 0;
    DeviceNode->QueryArbiterMask = 0;
    DeviceNode->QueryTranslatorMask = 0;
}

NTSTATUS
NTAPI
PipProcessStartPhase1(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG ReorderingBarrier)
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PipProcessStartPhase1: [%p] ReorderingBarrier %X\n", DeviceNode, ReorderingBarrier);

    ASSERT(DeviceNode->State == DeviceNodeResourcesAssigned);

    DeviceObject = DeviceNode->PhysicalDeviceObject;
    IopUncacheInterfaceInformation(DeviceObject);

    DPRINT("PipProcessStartPhase1: Dump ResourceList\n");
    PipDumpCmResourceList(DeviceNode->ResourceList, 1);

    DPRINT("PipProcessStartPhase1: Dump ResourceListTranslated\n");
    PipDumpCmResourceList(DeviceNode->ResourceListTranslated, 1);

    if (DeviceNode->DockInfo.DockStatus == DOCK_NOTDOCKDEVICE)
    {
        Status = IopStartDevice(DeviceNode);
        //DPRINT1("PipProcessStartPhase1: Status %X\n", Status);
    }
    else
    {
        DPRINT1("PipProcessStartPhase1: FIXME PpProfile...()`s\n");
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    PipSetDevNodeState(DeviceNode, DeviceNodeStartCompletion, FALSE);
    DeviceNode->CompletionStatus = Status;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PipProcessStartPhase2(
    _In_ PDEVICE_NODE DeviceNode)
{
    ULONG Problem;
    NTSTATUS CompletionStatus;

    PAGED_CODE();
    DPRINT("PipProcessStartPhase2: [%p] CompletionStatus %X\n", DeviceNode, DeviceNode->CompletionStatus);

    CompletionStatus = DeviceNode->CompletionStatus;

    if (DeviceNode->DockInfo.DockStatus)
    {
        DPRINT1("PipProcessStartPhase2: FIXME! [%p] DockStatus %X\n", DeviceNode, DeviceNode->DockInfo.DockStatus);
        ASSERT(FALSE); // IoDbgBreakPointEx();
    }

    if (!NT_SUCCESS(CompletionStatus))
    {
        DPRINT1("PipProcessStartPhase2: CompletionStatus %X\n", CompletionStatus);

        DeviceNode->DebugStatus = DeviceNode->CompletionStatus;

        if (CompletionStatus == STATUS_PNP_REBOOT_REQUIRED)
        {
            DPRINT1("PipProcessStartPhase2:Problem is CM_PROB_NEED_RESTART\n");
            Problem = CM_PROB_NEED_RESTART;
            ASSERT(FALSE); // IoDbgBreakPointEx();
        }
        else
        {
            DPRINT1("PipProcessStartPhase2:Problem is CM_PROB_FAILED_START\n");
            Problem = CM_PROB_FAILED_START;
            ASSERT(FALSE); // IoDbgBreakPointEx();
        }

        DPRINT1("PipProcessStartPhase2: Remove DeviceNode %p\n", DeviceNode);
        PipRequestDeviceRemoval(DeviceNode, FALSE, Problem);

        if (DeviceNode->DockInfo.DockStatus)
        {
            DPRINT1("PipProcessStartPhase2: FIXME! [%p] DockStatus %X\n", DeviceNode, DeviceNode->DockInfo.DockStatus);
            ASSERT(FALSE); // IoDbgBreakPointEx();
            ASSERT(DeviceNode->DockInfo.DockStatus == DOCK_QUIESCENT);
            //IoRequestDeviceEject(DeviceNode->PhysicalDeviceObject);
        }

        DPRINT1("PipProcessStartPhase2: return %X\n", CompletionStatus);
        return CompletionStatus;
    }

    IopDoDeferredSetInterfaceState(DeviceNode);

    if (IopBootConfigsReserved)
        goto Exit;

    if (DeviceNode->InterfaceType == InterfaceTypeUndefined)
        goto Exit;

    if (DeviceNode->InterfaceType == 1)
        IopAllocateLegacyBootResources(2, DeviceNode->BusNumber);

    IopAllocateLegacyBootResources(DeviceNode->InterfaceType, DeviceNode->BusNumber);

Exit:

    ASSERT(DeviceNode->State == DeviceNodeStartCompletion);
    PipSetDevNodeState(DeviceNode, DeviceNodeStartPostWork, NULL);

    return CompletionStatus;
}

NTSTATUS
NTAPI
PipProcessRestartPhase1(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN IsWait)
{
    PAGED_CODE();
    DPRINT("PipProcessRestartPhase1: DeviceNode %p\n", DeviceNode);

    ASSERT(DeviceNode->State == DeviceNodeStopped);

    DeviceNode->CompletionStatus = IopStartDevice(DeviceNode);
    PipSetDevNodeState(DeviceNode, DeviceNodeRestartCompletion, NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PipProcessRestartPhase2(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();
    DPRINT("PipProcessRestartPhase2: DeviceNode %p\n", DeviceNode);

    if (NT_SUCCESS(DeviceNode->CompletionStatus))
    {
        PipSetDevNodeState(DeviceNode, DeviceNodeStarted, NULL);
        return DeviceNode->CompletionStatus;
    }

    DPRINT1("PipProcessRestartPhase2: DeviceNode->CompletionStatus %X\n", DeviceNode->CompletionStatus);
    ASSERT(FALSE); // IoDbgBreakPointEx();

    return DeviceNode->CompletionStatus;
}

BOOLEAN
NTAPI
PiCollapseEnumRequests(
    _In_ PPIP_ENUM_REQUEST Request)
{
    DPRINT("PiCollapseEnumRequests: FIXME. Request - %p, RequestType - %X\n",
           Request, Request->RequestType);

    return FALSE;
}

NTSTATUS
NTAPI
PipProcessDevNodeTree(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN EnableLoadDriver,
    _In_ BOOLEAN ProcessFailedDevices,
    _In_ ULONG ReenumerationType, // (0|1|2)
    _In_ BOOLEAN IsWait,
    _In_ BOOLEAN ProcessOnlyIntermediateStates,
    _In_ SERVICE_LOAD_TYPE * DriverLoadType,
    _In_ PPIP_ENUM_REQUEST Request)
{
    PDEVICE_NODE CurrentNode = NULL;
    PDEVICE_NODE ParentNode = NULL;
    PDEVICE_NODE ResNode = NULL;
    PDEVICE_NODE StartNode;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DestinationString;
    WCHAR Buffer[260];
    PWSTR BufferInstance;
    PWSTR RootInstancePath;
    PWSTR Instance;
    PWSTR Path;
    PUSHORT pLength;
    ULONG NodeFlags;
    ULONG EnumStatus;
    ULONG ReenumAttempts = 0;
    NTSTATUS Status;
    NTSTATUS status;
    BOOLEAN IsAssigned;
    BOOLEAN IsCycle2End;
    BOOLEAN IsCycle1Run;

    PAGED_CODE();
    DPRINT("PipProcessDevNodeTree: DeviceNode %p\n", DeviceNode);
    DPRINT("LoadDrv %X ProcessFailed %X Type %X Wait %X\n", EnableLoadDriver, ProcessFailedDevices, ReenumerationType, IsWait);
    DPRINT("OnlyIntermed %X LoadType %X Request %X\n", ProcessOnlyIntermediateStates, (DriverLoadType ? *DriverLoadType : 0), Request);

    if (Request &&
        !Request->ReorderingBarrier &&
        ReenumerationType != 1 &&
        !ProcessOnlyIntermediateStates &&
        PiCollapseEnumRequests(Request))
    {
        StartNode = IopRootDeviceNode;
    }
    else
    {
        StartNode = DeviceNode;
    }

    IsCycle2End = FALSE;

    do
    {
        DPRINT("START1. CurrentNode %X\n", CurrentNode);

        IsCycle1Run = FALSE;

        if (!ProcessOnlyIntermediateStates)
        {
            IsAssigned = FALSE;

            DPRINT("ProcessFailedDevices %X\n", ProcessFailedDevices);

            IsCycle1Run = IopProcessAssignResources(StartNode, ProcessFailedDevices, &IsAssigned);

            DPRINT("IsCycle1Run %X IsAssigned %X\n", IsCycle1Run, IsAssigned);
    
            if (IsAssigned == TRUE)
            {
                status = PipProcessDevNodeTree(IopRootDeviceNode,
                                               EnableLoadDriver,
                                               FALSE, // ProcessFailedDevices
                                               ReenumerationType,
                                               IsWait,
                                               TRUE, // ProcessOnlyIntermediateStates
                                               DriverLoadType,
                                               Request);
                ASSERT(NT_SUCCESS(status));
            }
        }

        if (IsCycle2End == TRUE && IsCycle1Run == FALSE)
        {
            DPRINT("IsCycle2End == TRUE && IsCycle1Run == FALSE\n");
            break;
        }

        CurrentNode = StartNode;
        IsCycle2End = FALSE;

        do
        {
            DPRINT("[@] CurrentNode %p State %X\n", CurrentNode, CurrentNode->State);

            Status = STATUS_SUCCESS;
            EnumStatus = 1;

            NodeFlags = CurrentNode->Flags;
            if ((NodeFlags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM)) != 0)
            {
                DPRINT("CurrentNode->Flags %X\n", CurrentNode->Flags);
                goto NodeManager;
            }

            switch (CurrentNode->State)
            {
                case DeviceNodeUninitialized:
                {
                    DPRINT("[%p] DeviceNodeUninitialized\n", CurrentNode);
                    if (ProcessOnlyIntermediateStates)
                    {
                        DPRINT("[%p] ProcessOnlyIntermediateStates\n", CurrentNode);
                        goto NodeManager;
                    }

                    if (CurrentNode->Parent == ParentNode && !ResNode)
                    {
                        DPRINT("PipProcessDevNodeTree: ParentNode %p\n", ParentNode);
                        ResNode = CurrentNode;
                    }

                    if ((ProcessFailedDevices || ReenumerationType) && !ResNode)
                    {
                        DPRINT1("[%p] ReenumerationType %X ResNode %X\n", CurrentNode, ReenumerationType, ResNode);
                        goto NodeManager;
                    }

                    Status = PiProcessNewDeviceNode(CurrentNode);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("[%p] DeviceNodeUninitialized. Status %X\n", CurrentNode, Status);
                        ASSERT(FALSE); // IoDbgBreakPointEx();
                    }
                    else
                    {
                        EnumStatus = 0;
                    }

                    break;
                }
                case DeviceNodeInitialized:
                {
                    DPRINT("[%p] DeviceNodeInitialized\n", CurrentNode);
                    if (ProcessOnlyIntermediateStates || (ProcessFailedDevices && !ResNode))
                    {
                        DPRINT("[%p] ProcessFailedDevs %X ResNode %X\n", CurrentNode, ProcessFailedDevices, ResNode);
                        goto NodeManager;
                    }

                    Status = PipCallDriverAddDevice(CurrentNode, EnableLoadDriver, DriverLoadType);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT("[%p] DeviceNodeInitialized. Status %X\n", CurrentNode, Status);
                    }
                    else
                    {
                        EnumStatus = 0;
                        IsCycle1Run = TRUE;
                    }

                    break;
                }
                case DeviceNodeResourcesAssigned:
                {
                    DPRINT("[%p] DeviceNodeResourcesAssigned\n", CurrentNode);
                    if (ProcessOnlyIntermediateStates)
                    {
                        EnumStatus = 1;
                        break;
                    }

                    if (ProcessFailedDevices && !ResNode)
                        ResNode = CurrentNode;

                    Status = PipProcessStartPhase1(CurrentNode, IsWait);
                    if (NT_SUCCESS(Status))
                    {
                        EnumStatus = 0;
                        break;
                    }

                    DPRINT1("[%p] DeviceNodeResourcesAssigned. Status %X\n", CurrentNode, Status);
                    ASSERT(FALSE); // IoDbgBreakPointEx();
                    EnumStatus = 1;
                    break;
                }
                case DeviceNodeStartCompletion:
                {
                    DPRINT("[%p] DeviceNodeStartCompletion\n", CurrentNode);
                    Status = PipProcessStartPhase2(CurrentNode);
                    if (NT_SUCCESS(Status))
                    {
                        EnumStatus = 0;
                        break;
                    }

                    DPRINT1("[%p] StartCompletion State %X, Status %X\n", CurrentNode, CurrentNode->State, Status);

                    Status = STATUS_PNP_RESTART_ENUMERATION;
                    if (CurrentNode->State != DeviceNodeStartCompletion)
                    {
                        if (ProcessOnlyIntermediateStates)
                            goto NodeManager;

                        goto restartENUMERATION;
                    }

                    ASSERT(CurrentNode->State != DeviceNodeStartCompletion);
                    break;
                }
                case DeviceNodeStartPostWork:
                {
                    DPRINT("[%p] DeviceNodeStartPostWork\n", CurrentNode);
                    Status = PipProcessStartPhase3(CurrentNode);
                    if (NT_SUCCESS(Status))
                    {
                        EnumStatus = 0;
                        break;
                    }

                    DPRINT1("[%p] StartPostWork State %X, Status %X\n", CurrentNode, CurrentNode->State, Status);
                    Status = STATUS_PNP_RESTART_ENUMERATION;
                    if (!ProcessOnlyIntermediateStates)
                        goto restartENUMERATION;

                    ASSERT(!ProcessOnlyIntermediateStates);
                    break;
                }
                case DeviceNodeStarted:
                {
                    DPRINT("[%p] DeviceNodeStarted\n", CurrentNode);

                    EnumStatus = 2;

                    if (ProcessOnlyIntermediateStates || !(NodeFlags & 8))
                        goto NodeManager;

                    DPRINT("call IopQueryDeviceRelations, PDO %p\n", CurrentNode->PhysicalDeviceObject);
                    CurrentNode->Flags &= ~DNF_REENUMERATE;

                    Status = IopQueryDeviceRelations(BusRelations,
                                                     CurrentNode->PhysicalDeviceObject,
                                                     &CurrentNode->OverUsed1.PendingDeviceRelations);
                    if (Status == STATUS_PENDING)
                    {
                        DPRINT1("[%p] DeviceNodeStarted: STATUS_PENDING\n", CurrentNode);
                        ASSERT(FALSE); // IoDbgBreakPointEx();
                        EnumStatus = 1;
                        break;
                    }

                    if (NT_SUCCESS(Status))
                    {
                        EnumStatus = 0;
                        ParentNode = CurrentNode;
                        break;
                    }

                    DPRINT("Status %X EnumStatus %X\n", Status, EnumStatus);
                    break;
                }
                case DeviceNodeEnumerateCompletion:
                    DPRINT("[%p] DeviceNodeEnumerateCompletion\n", CurrentNode);
                    Status = PipEnumerateCompleted(CurrentNode);
                    EnumStatus = 2;
                    break;

                case DeviceNodeStopped:
                {
                    DPRINT("[%p] DeviceNodeStopped\n", CurrentNode);
                    Status = PipProcessRestartPhase1(CurrentNode, IsWait);
                    if (NT_SUCCESS(Status))
                    {
                        EnumStatus = 0;
                        break;
                    }

                    DPRINT1("[%p] DeviceNodeStopped. Status %X\n", CurrentNode, Status);
                    ASSERT(FALSE); // IoDbgBreakPointEx();
                    EnumStatus = 1;
                    break;
                }
                case DeviceNodeRestartCompletion:
                {
                    DPRINT("[%p] DeviceNodeRestartCompletion\n", CurrentNode);
                    Status = PipProcessRestartPhase2(CurrentNode);
                    if (NT_SUCCESS(Status))
                    {
                        EnumStatus = 0;
                        break;
                    }

                    DPRINT1("[%p] RestartCompletion State %X, Status %X\n", CurrentNode, CurrentNode->State, Status);
                    Status = STATUS_PNP_RESTART_ENUMERATION;
                    if (CurrentNode->State != DeviceNodeRestartCompletion)
                    {
                        if (ProcessOnlyIntermediateStates)
                            goto NodeManager;

                        goto restartENUMERATION;
                    }

                    ASSERT(CurrentNode->State != DeviceNodeRestartCompletion);
                    break;
                }
                case DeviceNodeDriversAdded:
                    DPRINT("[%p] DeviceNodeDriversAdded\n", CurrentNode);
                    EnumStatus = 1;
                    goto NodeManager;

                case DeviceNodeAwaitingQueuedDeletion:
                    DPRINT("[%p] DeviceNodeAwaitingQueuedDeletion\n", CurrentNode);
                    EnumStatus = 1;
                    goto NodeManager;

                case DeviceNodeAwaitingQueuedRemoval:
                    DPRINT("[%p] DeviceNodeAwaitingQueuedRemoval\n", CurrentNode);
                    EnumStatus = 1;
                    goto NodeManager;

                case DeviceNodeRemovePendingCloses:
                    DPRINT("[%p] DeviceNodeRemovePendingCloses\n", CurrentNode);
                    EnumStatus = 1;
                    goto NodeManager;

                case DeviceNodeRemoved:
                    DPRINT("[%p] DeviceNodeRemoved\n", CurrentNode);
                    EnumStatus = 1;
                    goto NodeManager;

                default:
                    DPRINT1("[%p] Unknown State %X\n", CurrentNode, CurrentNode->State);
                    ASSERT(FALSE); // IoDbgBreakPointEx();
                    EnumStatus = 1;
                    break;
            }

            if (Status != STATUS_PNP_RESTART_ENUMERATION)
                goto NodeManager;

            DPRINT1("STATUS_PNP_RESTART_ENUMERATION\n");

            if (ProcessOnlyIntermediateStates)
                goto NodeManager;

            DPRINT("ProcessOnlyIntermediateStates - FALSE\n");

restartENUMERATION:

            DPRINT("PipProcessDevNodeTree: restartENUMERATION\n");
            status = PipProcessDevNodeTree(IopRootDeviceNode,
                                           EnableLoadDriver,
                                           ProcessFailedDevices,
                                           ReenumerationType,
                                           IsWait,
                                           TRUE, // ProcessOnlyIntermediateStates
                                           DriverLoadType,
                                           Request);
            ASSERT(NT_SUCCESS(status));
            DPRINT("PipProcessDevNodeTree: end restartENUMERATION\n");

            DPRINT("FIXME PipAssertDevnodesInConsistentState\n");
            //PipAssertDevnodesInConsistentState();

            ReenumAttempts++;
            DPRINT("PipProcessDevNodeTree: ReenumAttempts %X\n", ReenumAttempts);

            if (ReenumAttempts >= STATUS_UNSUCCESSFUL)
            {
                RootInstancePath = NULL;
                Instance = NULL;
            }
            else
            {
                Instance = ExAllocatePoolWithTag(PagedPool, ((CurrentNode->Level + 1) * 2 * MAX_INSTANCE_PATH_LENGTH), 'nepP');
                if (!Instance)
                {
                    pLength = &StartNode->InstancePath.Length;
                    ASSERT(StartNode->InstancePath.Length < MAX_INSTANCE_PATH_LENGTH);

                    RtlCopyMemory(Buffer, StartNode->InstancePath.Buffer, *pLength);

                    RootInstancePath = Buffer;
                }
                else
                {
                    BufferInstance = Instance;
                    while (TRUE)
                    {
                        RootInstancePath = BufferInstance;
                        ASSERT(CurrentNode->InstancePath.Length < MAX_INSTANCE_PATH_LENGTH);

                        RtlCopyMemory(BufferInstance, CurrentNode->InstancePath.Buffer, CurrentNode->InstancePath.Length);

                        BufferInstance[CurrentNode->InstancePath.Length / 2] = 0;
                        BufferInstance = &BufferInstance[(CurrentNode->InstancePath.Length / 2 + 1)];

                        if (CurrentNode == StartNode)
                            break;

                        CurrentNode = CurrentNode->Parent;
                    }
                }
            }

            ObDereferenceObject(DeviceNode->PhysicalDeviceObject);

            PpDevNodeUnlockTree(1);
            PpSynchronizeDeviceEventQueue();
            PpDevNodeLockTree(1);
            DPRINT("PipProcessDevNodeTree: ReenumAttempts %X\n", ReenumAttempts);

            if (ReenumAttempts >= STATUS_UNSUCCESSFUL)
            {
                DPRINT("PipProcessDevNodeTree: Restarted reenumeration %d times, giving up!\n", ReenumAttempts);
                ASSERT(ReenumAttempts < MAX_REENUMERATION_ATTEMPTS);
                return STATUS_UNSUCCESSFUL;
            }

            RtlInitUnicodeString(&DestinationString, RootInstancePath);

            DeviceObject = IopDeviceObjectFromDeviceInstance(&DestinationString);
            if (!DeviceObject)
            {
                if (Instance)
                    ExFreePoolWithTag(Instance, 0);

                DPRINT("PipProcessDevNodeTree: return STATUS_SUCCESS\n");
                return STATUS_UNSUCCESSFUL;
            }

            DPRINT("PipProcessDevNodeTree: DeviceObject %p\n", DeviceObject);

            CurrentNode = IopGetDeviceNode(DeviceObject);
            StartNode = CurrentNode;
            DeviceNode = CurrentNode;

            if (!Instance)
            {
                EnumStatus = 0;
                goto NodeManager;
            }

            Path = Instance;
            if (Path == RootInstancePath)
            {
                ExFreePoolWithTag(Instance, 0);
                EnumStatus = 0;
                goto NodeManager;
            }

            while (TRUE)
            {
                RtlInitUnicodeString(&DestinationString, Path);

                DeviceObject = IopDeviceObjectFromDeviceInstance(&DestinationString);
                if (DeviceObject)
                    break;

                Path += ((DestinationString.Length / 2) + 1);
                if (Path == RootInstancePath)
                {
                    ExFreePoolWithTag(Instance, 0);
                    EnumStatus = 0;
                    goto NodeManager;
                }
            }

            CurrentNode = IopGetDeviceNode(DeviceObject);
            ObDereferenceObject(DeviceObject);

            ExFreePoolWithTag(Instance, 0);
            EnumStatus = 0;

NodeManager:
            DPRINT("NodeMgr [%p]->[%X] EnmSts %X\n", CurrentNode, CurrentNode->Child, EnumStatus);

            switch (EnumStatus)
            {
                case 0:
                    continue;
                    break;

                case 1:
                    break;

                case 2:
                    if (CurrentNode->Child)
                    {
                        CurrentNode = CurrentNode->Child;
                        continue;
                    }
                    break;

                default:
                    ASSERT(EnumStatus == 0 || EnumStatus == 1 || EnumStatus == 2);
                    break;
            }

            if (CurrentNode == StartNode)
            {
                IsCycle2End = TRUE;
                continue;
            }

            while (TRUE)
            {
                if (CurrentNode == ResNode)
                {
                    if (ReenumerationType)
                        ParentNode = ResNode->Parent;

                    ResNode = NULL;
                }
                else if (CurrentNode == ParentNode)
                {
                    ParentNode = ParentNode->Parent;
                }

                if (CurrentNode->Sibling)
                {
                    CurrentNode = CurrentNode->Sibling;
                    break;
                }

                if (CurrentNode->Parent)
                    CurrentNode = CurrentNode->Parent;

                if (CurrentNode == StartNode)
                    break;
            }

            if (CurrentNode == StartNode)
                IsCycle2End = TRUE;
        }
        while (IsCycle2End == FALSE);
    }
    while (IsCycle1Run == TRUE);

    if (!ProcessOnlyIntermediateStates)
    {
        DPRINT("FIXME PipAssertDevnodesInConsistentState\n");
        //PipAssertDevnodesInConsistentState();
        ObDereferenceObject(DeviceNode->PhysicalDeviceObject);
    }

    DPRINT("PipProcessDevNodeTree: return STATUS_SUCCESS\n");
    return STATUS_SUCCESS;
}

NTSTATUS
PiMarkDeviceTreeForReenumerationWorker(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PVOID Context)
{
    PAGED_CODE();

    if (DeviceNode->State != DeviceNodeStarted)
    {
        return STATUS_SUCCESS;
    }

    DeviceNode->Flags |= DNF_REENUMERATE;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiMarkDeviceTreeForReenumeration(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN EnumSubtree)
{
    DEVICETREE_TRAVERSE_CONTEXT Context;

    PAGED_CODE();

    PiMarkDeviceTreeForReenumerationWorker(DeviceNode, NULL);

    if (EnumSubtree == FALSE)
    {
        return STATUS_SUCCESS;
    }

    IopInitDeviceTreeTraverseContext(&Context,
                                     DeviceNode,
                                     PiMarkDeviceTreeForReenumerationWorker,
                                     DeviceNode);

   return IopTraverseDeviceTree(&Context);
}

NTSTATUS
NTAPI
PiProcessReenumeration(
    _In_ PPIP_ENUM_REQUEST Request)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    ULONG ReenumerationType;
    SERVICE_LOAD_TYPE DriverLoadType;

    PAGED_CODE();
    DPRINT("PiProcessReenumeration: Request - %p, Request->RequestType - %X\n",
           Request, Request->RequestType);

    DeviceObject = Request->DeviceObject;
    ASSERT(DeviceObject);

    DeviceNode = IopGetDeviceNode(DeviceObject);

    if (DeviceNode->State == DeviceNodeDeletePendingCloses ||
        DeviceNode->State == DeviceNodeDeleted)
    {
        ObDereferenceObject(DeviceObject);
        return STATUS_DELETE_PENDING;
    }

    if (Request->RequestType == PipEnumDeviceOnly)
    {
        ReenumerationType = PIP_REENUM_TYPE_SINGLE;
        PiMarkDeviceTreeForReenumeration(DeviceNode, FALSE);
    }
    else
    {
        // BusRelations
        ReenumerationType = PIP_REENUM_TYPE_SUBTREE;
        PiMarkDeviceTreeForReenumeration(DeviceNode, TRUE);
    }

    DriverLoadType = DemandLoad;

    PipProcessDevNodeTree(DeviceNode,
                          PnPBootDriversInitialized,
                          FALSE,
                          ReenumerationType,
                          TRUE,
                          FALSE,
                          &DriverLoadType,
                          Request);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiProcessAddBootDevices(
    _In_ PPIP_ENUM_REQUEST Request)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PiProcessResourceRequirementsChanged(
    _In_ PPIP_ENUM_REQUEST Request)
{
    PDEVICE_NODE DeviceNode;
    SERVICE_LOAD_TYPE DriverLoadType;
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(Request->DeviceObject);

    DeviceNode = IopGetDeviceNode(Request->DeviceObject);
    DPRINT("PiProcessResourceRequirementsChanged: %p, %p, %X\n", Request, DeviceNode, DeviceNode->State);

    if (DeviceNode->State == DeviceNodeDeletePendingCloses)
        return STATUS_DELETE_PENDING;

    if (DeviceNode->State == DeviceNodeDeleted)
        return STATUS_DELETE_PENDING;

    DeviceNode->Flags = ((DeviceNode->Flags & ~DNF_NO_RESOURCE_REQUIRED) | DNF_RESOURCE_REQUIREMENTS_CHANGED);

    PipClearDevNodeProblem(DeviceNode);

    if (DeviceNode->State != DeviceNodeStarted)
        return STATUS_UNSUCCESSFUL;

    if (Request->RequestArgument)
        DeviceNode->Flags &= ~DNF_NON_STOPPED_REBALANCE;
    else
        DeviceNode->Flags |= DNF_NON_STOPPED_REBALANCE;

    IopReallocateResources(DeviceNode);

    DriverLoadType = DemandLoad;
    Status = PipProcessDevNodeTree(IopRootDeviceNode,
                                   PnPBootDriversInitialized,
                                   FALSE,
                                   0,
                                   (Request->CompletionEvent != NULL),
                                   TRUE,
                                   &DriverLoadType,
                                   Request);
    if (NT_SUCCESS(Status))
        return Status;

    DPRINT("PiProcessResourceRequirementsChanged: Status %X\n", Status);
    ASSERT(NT_SUCCESS(Status));

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiProcessStartSystemDevices(
    _In_ PPIP_ENUM_REQUEST Request)
{
    SERVICE_LOAD_TYPE DriverLoadType = DemandLoad;
    PDEVICE_NODE DeviceNode;
    BOOLEAN IsWait;

    PAGED_CODE();
    DPRINT("PiProcessStartSystemDevices: Request %p\n", Request);

    DeviceNode = IopGetDeviceNode(Request->DeviceObject);

    IsWait = (Request->CompletionEvent != NULL);

    PipProcessDevNodeTree(DeviceNode,
                          PnPBootDriversInitialized,
                          FALSE,
                          0,
                          IsWait,
                          FALSE,
                          &DriverLoadType,
                          Request);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiProcessRequeryDeviceState(
    _In_ PPIP_ENUM_REQUEST Request)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
PiResetProblemDevicesWorker(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PVOID context)
{
    ULONG MaxUnload;
    PULONG Context = context;

    PAGED_CODE();
    DPRINT("PiResetProblemDevicesWorker: DeviceNode %p, State %X\n", DeviceNode, DeviceNode->State);

    if (!(DeviceNode->Flags & DNF_HAS_PROBLEM))
        return STATUS_SUCCESS;

    if (DeviceNode->Problem != Context[0])
        return STATUS_SUCCESS;

    MaxUnload = Context[2];

    if (MaxUnload)
    {
        if (DeviceNode->DriverUnloadRetryCount >= MaxUnload)
            return STATUS_SUCCESS;

        DeviceNode->DriverUnloadRetryCount++;
    }

    PipRequestDeviceAction(DeviceNode->PhysicalDeviceObject,
                           PipEnumGetSetDeviceStatus,
                           1,
                           0,
                           NULL,
                           NULL);

    if (!Context[1])
        return STATUS_SUCCESS;

    PipRequestDeviceAction(DeviceNode->PhysicalDeviceObject,
                           PipEnumStartDevice,
                           1,
                           0,
                           NULL,
                           NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PpProcessClearProblem(
    _In_ PPIP_ENUM_REQUEST Request)
{
    DEVICETREE_TRAVERSE_CONTEXT TraverseContext;
    PDEVICE_NODE DeviceNode;
    ULONG Context[3];

    PAGED_CODE();

    Context[0] = Request->RequestArgument;
    Context[1] = 1;
    Context[2] = 5;

    ASSERT(Request->DeviceObject);

    if (Request->DeviceObject)
        DeviceNode = IopGetDeviceNode(Request->DeviceObject);
    else
        DeviceNode = NULL;

    ASSERT(DeviceNode);

    DPRINT("PpProcessClearProblem: Request %p, DeviceNode %p, State %X\n", Request, DeviceNode, DeviceNode->State);

    if (DeviceNode->State == DeviceNodeDeletePendingCloses)
        return STATUS_DELETE_PENDING;

    if (DeviceNode->State == DeviceNodeDeleted)
        return STATUS_DELETE_PENDING;

    IopInitDeviceTreeTraverseContext(&TraverseContext,
                                     DeviceNode,
                                     PiResetProblemDevicesWorker,
                                     Context);

    IopTraverseDeviceTree(&TraverseContext);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PiProcessShutdownPnpDevices(
    _In_ PDEVICE_NODE DeviceNode)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IopRestartDeviceNode(
    _In_ PDEVICE_NODE DeviceNode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IopRestartDeviceNode: DeviceNode %X\n", DeviceNode);
    PAGED_CODE();

    PpDevNodeLockTree(3);

    ASSERT(DeviceNode->State == DeviceNodeRemoved ||
           DeviceNode->State == DeviceNodeInitialized);

    ASSERT(!(DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM)));

    if (!(DeviceNode->Flags & DNF_ENUMERATED))
    {
        PpDevNodeUnlockTree(3);
        DPRINT("IopRestartDeviceNode: return STATUS_UNSUCCESSFUL\n");
        ASSERT(DeviceNode->Flags & DNF_ENUMERATED);
        return STATUS_UNSUCCESSFUL;
    }

    DeviceNode->Flags &= ~(DNF_HARDWARE_VERIFICATION + DNF_DRIVER_BLOCKED);
    DeviceNode->UserFlags &= ~DNUF_NEED_RESTART;

    if (DeviceNode->State != DeviceNodeUninitialized)
    {
        DeviceNode->Flags &= ~(DNF_NO_RESOURCE_REQUIRED + DNF_RESOURCE_REQUIREMENTS_CHANGED);

        if (DeviceNode->ServiceName.Length)
        {
            ExFreePool(DeviceNode->ServiceName.Buffer);

            DeviceNode->ServiceName.MaximumLength = 0;
            DeviceNode->ServiceName.Length = 0;
            DeviceNode->ServiceName.Buffer = NULL;
        }

        if (DeviceNode->ResourceRequirements)
        {
            ExFreePool(DeviceNode->ResourceRequirements);

            DeviceNode->Flags &= ~DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
            DeviceNode->ResourceRequirements = NULL;
        }
    }

    ASSERT(DeviceNode->ServiceName.Length == 0 &&
           DeviceNode->ServiceName.MaximumLength == 0 &&
           DeviceNode->ServiceName.Buffer == NULL);

    ASSERT(!(DeviceNode->Flags & ~(DNF_MADEUP | DNF_ENUMERATED | DNF_IDS_QUERIED | DNF_HAS_BOOT_CONFIG |
                                   DNF_BOOT_CONFIG_RESERVED | DNF_NO_RESOURCE_REQUIRED)));

    PipSetDevNodeState(DeviceNode, DeviceNodeUninitialized, NULL);
    PpDevNodeUnlockTree(3);

    return Status;
}

NTSTATUS
NTAPI
PiRestartDevice(
    _In_ PPIP_ENUM_REQUEST Request)
{
    SERVICE_LOAD_TYPE DriverLoadType;
    PDEVICE_NODE DeviceNode;

    PAGED_CODE();

    ASSERT(Request->DeviceObject != NULL);

    DeviceNode = IopGetDeviceNode(Request->DeviceObject);
    DPRINT("PiRestartDevice: DeviceNode %X, State %X\n", DeviceNode, DeviceNode->State);

    if (DeviceNode->State == DeviceNodeDeletePendingCloses ||
        DeviceNode->State == DeviceNodeDeleted)
    {
        DPRINT("PiRestartDevice: return STATUS_DELETE_PENDING\n");
        return STATUS_DELETE_PENDING;
    }

    if (DeviceNode->Flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM))
    {
        DPRINT("PiRestartDevice: return STATUS_UNSUCCESSFUL\n");
        return STATUS_UNSUCCESSFUL;
    }

    switch (DeviceNode->State)
    {
        case DeviceNodeStartPending:
            DPRINT1("PiRestartDevice: DeviceNodeStartPending. STATUS_SUCCESS\n");
            ASSERT(FALSE); // IoDbgBreakPointEx();
            return STATUS_SUCCESS;

        case DeviceNodeInitialized:
            DPRINT("PiRestartDevice: DeviceNodeInitialized\n");
            ASSERT(!(DeviceNode->UserFlags & DNUF_WILL_BE_REMOVED));
            IopRestartDeviceNode(DeviceNode);
            break;

        case DeviceNodeRemoved:
            DPRINT("PiRestartDevice: DeviceNodeRemoved\n");
            ASSERT(!(DeviceNode->UserFlags & DNUF_WILL_BE_REMOVED));
            IopRestartDeviceNode(DeviceNode);
            break;

        case DeviceNodeUninitialized:
            DPRINT("PiRestartDevice: DeviceNodeUninitialized\n");
            break;

        case DeviceNodeDriversAdded:
            DPRINT("PiRestartDevice: DeviceNodeDriversAdded\n");
            break;

        case DeviceNodeResourcesAssigned:
            DPRINT("PiRestartDevice: DeviceNodeResourcesAssigned\n");
            break;

        case DeviceNodeStartCompletion:
            DPRINT("PiRestartDevice: DeviceNodeStartCompletion\n");
            break;

        case DeviceNodeStartPostWork:
            DPRINT("PiRestartDevice: DeviceNodeStartPostWork\n");
            break;

        case DeviceNodeEnumerateCompletion:
            DPRINT("PiRestartDevice: DeviceNodeEnumerateCompletion\n");
            break;

        case DeviceNodeStarted:
            DPRINT("PiRestartDevice: DeviceNodeStarted. STATUS_SUCCESS\n");
            return STATUS_SUCCESS;

        case DeviceNodeQueryStopped:
            DPRINT("PiRestartDevice: DeviceNodeQueryStopped. STATUS_SUCCESS\n");
            return STATUS_SUCCESS;

        case DeviceNodeStopped:
            DPRINT("PiRestartDevice: DeviceNodeStopped. STATUS_SUCCESS\n");
            return STATUS_SUCCESS;

        case DeviceNodeRestartCompletion:
            DPRINT("PiRestartDevice: DeviceNodeRestartCompletion. STATUS_SUCCESS\n");
            return STATUS_SUCCESS;

        case DeviceNodeEnumeratePending:
            DPRINT("PiRestartDevice: DeviceNodeEnumeratePending. STATUS_SUCCESS\n");
            return STATUS_SUCCESS;

        case DeviceNodeAwaitingQueuedDeletion:
            DPRINT("PiRestartDevice: DeviceNodeAwaitingQueuedDeletion. STATUS_UNSUCCESSFUL\n");
            return STATUS_UNSUCCESSFUL;

        case DeviceNodeAwaitingQueuedRemoval:
            DPRINT("PiRestartDevice: DeviceNodeAwaitingQueuedRemoval. STATUS_UNSUCCESSFUL\n");
            return STATUS_UNSUCCESSFUL;

        case DeviceNodeQueryRemoved:
            DPRINT("PiRestartDevice: DeviceNodeQueryRemoved. STATUS_UNSUCCESSFUL\n");
            return STATUS_UNSUCCESSFUL;

        case DeviceNodeRemovePendingCloses:
            DPRINT("PiRestartDevice: DeviceNodeRemovePendingCloses. STATUS_UNSUCCESSFUL\n");
            return STATUS_UNSUCCESSFUL;

        case DeviceNodeDeletePendingCloses:
            DPRINT("PiRestartDevice: DeviceNodeDeletePendingCloses. STATUS_UNSUCCESSFUL\n");
            return STATUS_UNSUCCESSFUL;

        default:
            DPRINT1("PiRestartDevice: Unknown State %X. STATUS_UNSUCCESSFUL\n", DeviceNode->State);
            ASSERT(FALSE); // IoDbgBreakPointEx();
            return STATUS_UNSUCCESSFUL;
    }

    if (Request->RequestType != PipEnumStartDevice)
        return STATUS_SUCCESS;

    ObReferenceObject(DeviceNode->PhysicalDeviceObject);

    DriverLoadType = DemandLoad;

    PipProcessDevNodeTree(DeviceNode,
                          PnPBootDriversInitialized,
                          FALSE,
                          0,
                          (Request->CompletionEvent != NULL),
                          FALSE,
                          &DriverLoadType,
                          Request);

    return STATUS_SUCCESS;
}

VOID
NTAPI
PipEnumerationWorker(
    _In_ PVOID Context)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;
    PPIP_ENUM_REQUEST Request;
    BOOLEAN IsDereferenceObject;
    BOOLEAN IsBootProcess = FALSE;
    BOOLEAN IsAssignResources = FALSE;
    KIRQL OldIrql;
    NTSTATUS Status;

    PpDevNodeLockTree(1);

    while (TRUE)
    {
        Status = STATUS_SUCCESS;
        IsDereferenceObject = TRUE;

        KeAcquireSpinLock(&IopPnPSpinLock, &OldIrql);

        Request = CONTAINING_RECORD(IopPnpEnumerationRequestList.Flink,
                                    PIP_ENUM_REQUEST,
                                    RequestLink);

        if (IsListEmpty(&IopPnpEnumerationRequestList))
            break;

        RemoveHeadList(&IopPnpEnumerationRequestList);

Start:
        KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

        if (Request)
        {
            InitializeListHead(&Request->RequestLink);

            if (PpPnpShuttingDown)
            {
                DPRINT("PipEnumerationWorker: STATUS_TOO_LATE (PpPnpShuttingDown)\n");
                Status = STATUS_TOO_LATE;
                goto NextRequest;
            }

            DeviceObject = Request->DeviceObject;
            ASSERT(DeviceObject);

            DeviceNode = IopGetDeviceNode(DeviceObject);
            ASSERT(DeviceNode);

            if (DeviceNode->State == DeviceNodeDeleted)
            {
                DPRINT("PipEnumerationWorker: DeviceNode->State == DeviceNodeDeleted\n");
                Status = STATUS_UNSUCCESSFUL;
                goto NextRequest;
            }

            DPRINT("PipEnumerationWorker: DeviceObject - %p, Request->RequestType - %X\n",
                   DeviceObject,
                   Request->RequestType);

            switch (Request->RequestType)
            {
                case PipEnumDeviceOnly:
                case PipEnumDeviceTree:
                case PipEnumRootDevices:
                case PipEnumSystemHiveLimitChange:
                    DPRINT("PipEnumerationWorker: Reenumeration ...\n");
                    Status = PiProcessReenumeration(Request);
                    IsDereferenceObject = FALSE;
                    break;

                case PipEnumAddBootDevices:
                    DPRINT("PipEnumerationWorker: PipEnumAddBootDevices\n");
                    Status = PiProcessAddBootDevices(Request);
                    DPRINT("PipEnumerationWorker: end\n");
                    break;

                case PipEnumBootDevices:
                    DPRINT("PipEnumerationWorker: PipEnumBootDevices\n");
                    IsBootProcess = TRUE;
                    Request = NULL;
                    goto Start;

                case PipEnumAssignResources:
                    DPRINT("PipEnumerationWorker: PipEnumAssignResources\n");
                    IsAssignResources = TRUE;
                    Request = NULL;
                    goto Start;

                case PipEnumGetSetDeviceStatus:
                    DPRINT1("PipEnumerationWorker: PipEnumGetSetDeviceStatus\n");
                    ASSERT(FALSE);
                    break;

                case PipEnumInvalidateRelationsInList:
                    DPRINT1("PipEnumerationWorker: PipEnumInvalidateRelationsInList\n");
                    ASSERT(FALSE);
                    break;

                case PipEnumClearProblem:
                    DPRINT("PipEnumerationWorker: PipEnumClearProblem\n");
                    Status = PpProcessClearProblem(Request);
                    break;

                case PipEnumHaltDevice:
                    DPRINT1("PipEnumerationWorker: PipEnumHaltDevice\n");
                    ASSERT(FALSE);
                    break;

                case PipEnumInvalidateDeviceState:
                    DPRINT("PipEnumerationWorker: PipEnumInvalidateDeviceState\n");
                    Status = PiProcessRequeryDeviceState(Request);
                    break;

                case PipEnumResetDevice:
                    DPRINT("PipEnumerationWorker: PipEnumResetDevice\n");
                    goto RestartDevice;

                case PipEnumStartDevice:
                    DPRINT("PipEnumerationWorker: PipEnumStartDevice\n");
RestartDevice:
                    Status = PiRestartDevice(Request);
                    break;

                case PipEnumIoResourceChanged:
                    DPRINT("PipEnumerationWorker: PipEnumIoResourceChanged\n");
                    Status = PiProcessResourceRequirementsChanged(Request);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("PipEnumerationWorker: Status %X\n", Status);
                        ASSERT(FALSE);
                        IsAssignResources = TRUE;
                        Status = STATUS_SUCCESS;
                        Request = NULL;
                        goto Start;
                    }
                    break;

                case PipEnumSetProblem:
                    DPRINT1("PipEnumerationWorker: PipEnumSetProblem\n");
                    ASSERT(FALSE);
                    break;

                case PipEnumShutdownPnpDevices:
                    DPRINT("PipEnumerationWorker: PipEnumShutdownPnpDevices\n");
                    Status = PiProcessShutdownPnpDevices(IopRootDeviceNode);
                    break;

                case PipEnumStartSystemDevices:
                    DPRINT("PipEnumerationWorker: PipEnumStartSystemDevices\n");
                    Status = PiProcessStartSystemDevices(Request);
                    IsDereferenceObject = FALSE;
                    break;

                default:
                    DPRINT1("PipEnumerationWorker: unknown RequestType %X\n", Request->RequestType);
                    ASSERT(FALSE);
                    break;
            }

NextRequest:

            if (Request->CompletionStatus)
                *Request->CompletionStatus = Status;

            if (Request->CompletionEvent)
                KeSetEvent(Request->CompletionEvent, IO_NO_INCREMENT, FALSE);

            if (IsDereferenceObject)
                ObDereferenceObject(Request->DeviceObject);

            ExFreePoolWithTag(Request, TAG_IO);
        }
        else if (IsAssignResources || IsBootProcess)
        {
            SERVICE_LOAD_TYPE DriverLoadType = DemandLoad;

            ObReferenceObject(IopRootDeviceNode->PhysicalDeviceObject);

            PipProcessDevNodeTree(IopRootDeviceNode,
                                  PnPBootDriversInitialized,
                                  IsAssignResources,
                                  0,
                                  FALSE,
                                  FALSE,
                                  &DriverLoadType,
                                  NULL);

            IsAssignResources = FALSE;
            IsBootProcess = FALSE;
        }
        else
        {
            DPRINT1("PipEnumerationWorker: Error! (%X)\n", IsListEmpty(&IopPnpEnumerationRequestList));
            ASSERT(FALSE);
        }
    }

    PipEnumerationInProgress = FALSE;
    KeSetEvent(&PiEnumerationLock, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);
    PpDevNodeUnlockTree(1);
}

NTSTATUS
NTAPI
PipRequestDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIP_ENUM_TYPE RequestType,
    _In_ UCHAR ReorderingBarrier,
    _In_ ULONG_PTR RequestArgument,
    _In_ PKEVENT CompletionEvent,
    _Inout_ NTSTATUS * CompletionStatus)
{
    PPIP_ENUM_REQUEST Request;
    PDEVICE_OBJECT RequestDeviceObject;
    KIRQL OldIrql;

    DPRINT("PipRequestDeviceAction: DeviceObject - %p, RequestType - %X\n",
           DeviceObject,
           RequestType);

    if (PpPnpShuttingDown)
    {
        DPRINT1("PipRequestDeviceAction: STATUS_TOO_LATE\n");
        return STATUS_TOO_LATE;
    }

    Request = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(PIP_ENUM_REQUEST),
                                    TAG_IO);
    if (!Request)
    {
        DPRINT1("PipRequestDeviceAction: STATUS_INSUFFICIENT_RESOURCES\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!DeviceObject)
    {
        RequestDeviceObject = IopRootDeviceNode->PhysicalDeviceObject;
    }
    else
    {
        RequestDeviceObject = DeviceObject;
    }

    ObReferenceObject(RequestDeviceObject);

    Request->DeviceObject = RequestDeviceObject;
    Request->RequestType = RequestType;
    Request->ReorderingBarrier = ReorderingBarrier;
    Request->RequestArgument = RequestArgument;
    Request->CompletionEvent = CompletionEvent;
    Request->CompletionStatus = CompletionStatus;

    InitializeListHead(&Request->RequestLink);

    KeAcquireSpinLock(&IopPnPSpinLock, &OldIrql);

    InsertTailList(&IopPnpEnumerationRequestList, &Request->RequestLink);
    DPRINT("PipRequestDeviceAction: Inserted Request - %p\n", Request);

    if (RequestType == PipEnumAddBootDevices ||
        RequestType == PipEnumBootDevices ||
        RequestType == PipEnumRootDevices)
    {
        ASSERT(!PipEnumerationInProgress);

        PipEnumerationInProgress = TRUE;
        KeClearEvent(&PiEnumerationLock);
        KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

        PipEnumerationWorker(Request);

        return STATUS_SUCCESS;
    }

    if (!PnPBootDriversLoaded)
    {
        DPRINT("PipRequestDeviceAction: PnPBootDriversLoaded - FALSE\n");
        KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);
        return STATUS_SUCCESS;
    }

    if (PipEnumerationInProgress)
    {
        DPRINT("PipRequestDeviceAction: PipEnumerationInProgress - TRUE\n");
        KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);
        return STATUS_SUCCESS;
    }

    PipEnumerationInProgress = TRUE;
    KeClearEvent(&PiEnumerationLock);
    KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

    ExInitializeWorkItem(&PipDeviceEnumerationWorkItem,
                         PipEnumerationWorker,
                         Request);

    ExQueueWorkItem(&PipDeviceEnumerationWorkItem, DelayedWorkQueue);
    DPRINT("PipRequestDeviceAction: Queue &PipDeviceEnumerationWorkItem - %p\n",
           &PipDeviceEnumerationWorkItem);

    return STATUS_SUCCESS;
}

VOID
NTAPI
IoInvalidateDeviceState(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);

    DPRINT("IoInvalidateDeviceState: PhysicalDeviceObject - %p\n", PhysicalDeviceObject);

    if (PhysicalDeviceObject == NULL ||
        DeviceNode == NULL ||
        DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)
    {
        DPRINT1("IoInvalidateDeviceState: PNP_DETECTED_FATAL_ERROR\n");
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR,
                     2,
                     (ULONG_PTR)PhysicalDeviceObject,
                     0,
                     0);
    }

    if (DeviceNode->State != DeviceNodeStarted)
    {
        DPRINT("IoInvalidateDeviceState: DeviceNode->State - %X\n", DeviceNode->State);
        return;
    }

    PipRequestDeviceAction(PhysicalDeviceObject,
                           PipEnumInvalidateDeviceState,
                           0,
                           0,
                           NULL,
                           NULL);
}

VOID
NTAPI
IoInvalidateDeviceRelations(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_RELATION_TYPE Type)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);

    if (!DeviceObject ||
        !DeviceNode ||
        DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)
    {
        DPRINT1("IoInvalidateDeviceRelations: %p, %X\n", DeviceObject, Type);
        ASSERT(FALSE);
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG_PTR)DeviceObject, 0x0, 0x0);
    }

    switch (Type)
    {
        case BusRelations:
        {
            DPRINT("IoInvalidateDeviceRelations: [%p] BusRelations (PipEnumDeviceTree)\n", DeviceObject);
            PipRequestDeviceAction(DeviceObject,
                                   PipEnumDeviceTree,
                                   0,
                                   0,
                                   NULL,
                                   NULL);
            break;
        }
        case PowerRelations:
        {
            DPRINT1("IoInvalidateDeviceRelations: [%p] PowerRelations NOT_IMPLEMENTED FIXME!\n", DeviceObject);
            //PoInvalidateDevicePowerRelations(DeviceObject);
            break;
        }
        case SingleBusRelations:
        {
            DPRINT("IoInvalidateDeviceRelations: [%p] SingleBusRelations (PipEnumDeviceOnly)\n", DeviceObject);
            PipRequestDeviceAction(DeviceObject,
                                   PipEnumDeviceOnly,
                                   0,
                                   0,
                                   NULL,
                                   NULL);
            break;
        }
        default:
        {
            DPRINT1("IoInvalidateDeviceRelations: [%p] Type %X not supported\n", DeviceObject, Type);
            break;
        }

    }
}

NTSTATUS
NTAPI
IoSynchronousInvalidateDeviceRelations(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_RELATION_TYPE Type)
{
    DPRINT1("IoSynchronousInvalidateDeviceRelations: FIXME!\n");
    ASSERT(FALSE);
    return STATUS_NOT_SUPPORTED;
}

/* EOF */

#ifndef _PNPIO_H
#define _PNPIO_H

#define TAG_RTLREGISTRY 'vrqR'

#define IO_REG_KEY_RESOURCEMAP        L"\\REGISTRY\\MACHINE\\HARDWARE\\RESOURCEMAP"
#define IO_REG_KEY_DESCRIPTIONSYSTEM  L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM"

#define IO_REG_KEY_MACHINESYSTEM      L"\\REGISTRY\\MACHINE\\SYSTEM"
#define IO_REG_KEY_CURRENTCONTROLSET  L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET"
#define IO_REG_KEY_BOOTLOG            L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\BOOTLOG"
#define IO_REG_KEY_ARBITERS           L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\ARBITERS"
#define IO_REG_KEY_CONTROLCLASS       L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\CLASS"
#define IO_REG_KEY_CRITICALDEVICEDB   L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\CRITICALDEVICEDATABASE"
#define IO_REG_KEY_DEVICECLASSES      L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\DEVICECLASSES"
#define IO_REG_KEY_GROUPORDERLIST     L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\GROUPORDERLIST"
#define IO_REG_KEY_SERVICEGROUPORDER  L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\SERVICEGROUPORDER"
#define IO_REG_KEY_ENUM               L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\ENUM"
#define IO_REG_KEY_ENUMROOT           L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\ENUM\\ROOT"
#define IO_REG_KEY_HWPROFILESCURRENT  L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\HARDWARE PROFILES\\CURRENT"
#define IO_REG_KEY_SERVICES           L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES"

#define MAX_INSTANCE_PATH_LENGTH    260
#define MAX_REENUMERATION_ATTEMPTS  32

#define MAX_DEVICE_ID_LEN           200
#define MAX_SEPARATORS_INSTANCEID   0
#define MAX_SEPARATORS_DEVICEID     1
#define MAX_SEPARATORS_MULTI_SZ    -1

#define PIP_SUBKEY_FLAG_SKIP_ERROR  1
#define PIP_SUBKEY_FLAG_DELETE_KEY  2

#define PIP_REENUM_TYPE_SINGLE      1
#define PIP_REENUM_TYPE_SUBTREE     2

#define IOP_RES_HANDLER_TYPE_TRANSLATOR 1
#define IOP_RES_HANDLER_TYPE_ARBITER    2
#define IOP_RES_HANDLER_TYPE_LEGACY     3
#define IOP_MAX_MAIN_RESOURCE_TYPE      15

/* For Resource Lists */
#define PIP_CONFIG_TYPE_ALLOC   1
#define PIP_CONFIG_TYPE_FORCED  2
#define PIP_CONFIG_TYPE_BOOT    4

/* For Resource Requirements Lists */
#define PIP_CONFIG_TYPE_BASIC     1
#define PIP_CONFIG_TYPE_FILTERED  2
#define PIP_CONFIG_TYPE_OVERRIDE  4

/* Dump resources flags */
#define PIP_DUMP_FL_ALL_NODES        1
#define PIP_DUMP_FL_RES_ALLOCATED    2
#define PIP_DUMP_FL_RES_REQUIREMENTS 4
#define PIP_DUMP_FL_RES_TRANSLATED   8

typedef struct _PNP_DEVICE_INSTANCE_CONTEXT
{
    PDEVICE_OBJECT DeviceObject;
    PUNICODE_STRING InstancePath;
} PNP_DEVICE_INSTANCE_CONTEXT, *PPNP_DEVICE_INSTANCE_CONTEXT;

typedef enum _PIP_TYPE_REMOVAL_DEVICE
{
    PipQueryRemove,
    PipCancelRemove,
    PipRemove,
    PipSurpriseRemove,
    PipEject,
    PipRemoveFailed,
    PipRemoveFailedNotStarted
} PIP_TYPE_REMOVAL_DEVICE;

/* Request types for PIP_ENUM_REQUEST */
typedef enum _PIP_ENUM_TYPE
{
    PipEnumAddBootDevices,
    PipEnumAssignResources,//PipEnumResourcesAssign
    PipEnumGetSetDeviceStatus,
    PipEnumClearProblem,
    PipEnumInvalidateRelationsInList,
    PipEnumHaltDevice,
    PipEnumBootDevices,//PipEnumBootProcess
    PipEnumDeviceOnly,//PipEnumInvalidateRelations
    PipEnumDeviceTree,//PipEnumInvalidateBusRelations
    PipEnumRootDevices,//PipEnumInitPnpServices
    PipEnumInvalidateDeviceState,
    PipEnumResetDevice,
    PipEnumIoResourceChanged,//PipEnumResourceChange
    PipEnumSystemHiveLimitChange,
    PipEnumSetProblem,
    PipEnumShutdownPnpDevices,
    PipEnumStartDevice,
    PipEnumStartSystemDevices
} PIP_ENUM_TYPE;

typedef struct _PIP_ENUM_REQUEST
{
    LIST_ENTRY RequestLink;
    PDEVICE_OBJECT DeviceObject;
    PIP_ENUM_TYPE RequestType;
    UCHAR ReorderingBarrier;
    UCHAR Padded[3];
    ULONG_PTR RequestArgument;
    PKEVENT CompletionEvent;
    NTSTATUS * CompletionStatus;
} PIP_ENUM_REQUEST, *PPIP_ENUM_REQUEST;

typedef struct _PI_RESOURCE_TRANSLATOR_ENTRY
{
    LIST_ENTRY DeviceTranslatorList; // Link to DeviceNode->DeviceTranslatorList
    UCHAR ResourceType;
    UCHAR Padding[3];
    PTRANSLATOR_INTERFACE TranslatorInterface;
    PDEVICE_NODE DeviceNode;
} PI_RESOURCE_TRANSLATOR_ENTRY, *PPI_RESOURCE_TRANSLATOR_ENTRY;

typedef struct _PNP_REQ_LIST PNP_REQ_LIST, *PPNP_REQ_LIST;

typedef struct _PNP_RESOURCE_REQUEST
{
    PDEVICE_OBJECT PhysicalDevice;
    ULONG Flags;
    ARBITER_REQUEST_SOURCE AllocationType;
    ULONG Priority;
    ULONG Position;
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements;
    PPNP_REQ_LIST ReqList;
    PCM_RESOURCE_LIST ResourceAssignment;
    PCM_RESOURCE_LIST TranslatedResourceAssignment;
    NTSTATUS Status;
} PNP_RESOURCE_REQUEST, *PPNP_RESOURCE_REQUEST;

typedef struct _PNP_REQ_RESOURCE_ENTRY
{
    LIST_ENTRY Link; // Link to (PPI_RESOURCE_ARBITER_ENTRY)->ResourceList
    ULONG Count;
    PIO_RESOURCE_DESCRIPTOR IoDescriptor;
    PDEVICE_OBJECT PhysicalDevice;
    ARBITER_REQUEST_SOURCE AllocationType;
    ULONG Reserved1;
    ULONG Reserved2;
    INTERFACE_TYPE InterfaceType;
    ULONG SlotNumber;
    ULONG BusNumber;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pCmDescriptor;
    ULONG Reserved3;
    ULONG Reserved4;
    CM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
} PNP_REQ_RESOURCE_ENTRY, *PPNP_REQ_RESOURCE_ENTRY;

typedef struct _PNP_REQ_ALT_LIST *PPNP_REQ_ALT_LIST;

typedef struct _PNP_REQ_DESCRIPTOR
{
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    BOOLEAN IsArbitrated;
    UCHAR Padded0[3];
    PPNP_REQ_ALT_LIST AltList;
    ULONG DescNumber;
    struct _PNP_REQ_DESCRIPTOR * TranslatedReqDesc;
    PNP_REQ_RESOURCE_ENTRY ReqEntry;
    ULONG Reserved[18];
    ULONG DescriptorsCount;
    PIO_RESOURCE_DESCRIPTOR DevicePrivateIoDesc; // CmResourceTypeDevicePrivate
    union
    {
        struct _PI_RESOURCE_ARBITER_ENTRY * ArbiterEntry;
        struct _PI_RESOURCE_TRANSLATOR_ENTRY * TranslatorEntry;
    };
} PNP_REQ_DESCRIPTOR, *PPNP_REQ_DESCRIPTOR;

typedef struct _PNP_REQ_ALT_LIST
{
    ULONG ConfigPriority;
    ULONG Priority;
    PPNP_REQ_LIST ReqList;
    ULONG ListNumber;
    ULONG CountDescriptors;
    PPNP_REQ_DESCRIPTOR ReqDescriptors[1]; // array pointers to descriptors
} PNP_REQ_ALT_LIST, *PPNP_REQ_ALT_LIST;

typedef struct _PNP_REQ_LIST
{ 
    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;
    PPNP_RESOURCE_REQUEST ResRequest;
    PPNP_REQ_ALT_LIST * AltList1;
    PPNP_REQ_ALT_LIST * AltList2;
    ULONG Count;
    PPNP_REQ_ALT_LIST AltLists[1]; // array pointers to alternative lists
} PNP_REQ_LIST, *PPNP_REQ_LIST;

typedef struct _PIP_ASSIGN_RESOURCES_CONTEXT
{
    ULONG DeviceCount;
    BOOLEAN IncludeFailedDevices;
    UCHAR Padded[3];
    PDEVICE_OBJECT DeviceList[1];
} PIP_ASSIGN_RESOURCES_CONTEXT, *PPIP_ASSIGN_RESOURCES_CONTEXT;

typedef BOOLEAN
(NTAPI * PIP_FUNCTION_TO_SUBKEYS)(
    _In_ HANDLE Handle,
    _In_ PUNICODE_STRING Name,
    _In_ PVOID Context
);

typedef
NTSTATUS
(NTAPI * PNP_ALLOCATE_RESOURCES_ROUTINE)(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST CmResource
);

typedef struct _PNP_RESERVED_RESOURCES_CONTEXT
{
    struct _PNP_RESERVED_RESOURCES_CONTEXT * NextReservedContext;
    PDEVICE_OBJECT DeviceObject;
    PCM_RESOURCE_LIST ReservedResource;
} PNP_RESERVED_RESOURCES_CONTEXT, *PPNP_RESERVED_RESOURCES_CONTEXT;

typedef
BOOLEAN
(NTAPI * PIP_CRITICAL_CALLBACK_VERIFY_CRITICAL_ENTRY)(
    _In_ HANDLE KeyHandle
);

typedef union _DEVICE_CAPABILITIES_FLAGS
{
    struct {
        ULONG  DeviceD1:1;
        ULONG  DeviceD2:1;
        ULONG  LockSupported:1;
        ULONG  EjectSupported:1;
        ULONG  Removable:1;
        ULONG  DockDevice:1;
        ULONG  UniqueID:1;
        ULONG  SilentInstall:1;
        ULONG  RawDeviceOK:1;
        ULONG  SurpriseRemovalOK:1;
        ULONG  WakeFromD0:1;
        ULONG  WakeFromD1:1;
        ULONG  WakeFromD2:1;
        ULONG  WakeFromD3:1;
        ULONG  HardwareDisabled:1;
        ULONG  NonDynamic:1;
        ULONG  WarmEjectSupported:1;
        ULONG  NoDisplayInUI:1;
        ULONG  Reserved:14;
    };
    ULONG AsULONG;
} DEVICE_CAPABILITIES_FLAGS, *PDEVICE_CAPABILITIES_FLAGS;

C_ASSERT(sizeof(DEVICE_CAPABILITIES_FLAGS) == sizeof(ULONG));

typedef union _CM_DEVICE_CAPABILITIES_FLAGS
{
    struct {
        ULONG  LockSupported:1;
        ULONG  EjectSupported:1;
        ULONG  Removable:1;
        ULONG  DockDevice:1;
        ULONG  UniqueID:1;
        ULONG  SilentInstall:1;
        ULONG  RawDeviceOK:1;
        ULONG  SurpriseRemovalOK:1;
        ULONG  HardwareDisabled:1;
        ULONG  NonDynamic:1;
        ULONG  Reserved:22;
    };
    ULONG AsULONG;
} CM_DEVICE_CAPABILITIES_FLAGS, *PCM_DEVICE_CAPABILITIES_FLAGS;

C_ASSERT(sizeof(CM_DEVICE_CAPABILITIES_FLAGS) == sizeof(ULONG));

typedef struct _IOPNP_DEVICE_EXTENSION
{
    PWCHAR CompatibleIdList;
    ULONG CompatibleIdListSize;
} IOPNP_DEVICE_EXTENSION, *PIOPNP_DEVICE_EXTENSION;

typedef struct _OBJECT_NAME_INFORMATION_EX
{
    UNICODE_STRING Name;
    WCHAR Buffer[252];
} OBJECT_NAME_INFORMATION_EX, *POBJECT_NAME_INFORMATION_EX;

typedef struct _DEVNODE_INTERFACE_STATE
{ 
    LIST_ENTRY Link; 
    UNICODE_STRING SymbolicLinkName; 
} DEVNODE_INTERFACE_STATE, *PDEVNODE_INTERFACE_STATE; 

typedef NTSTATUS
(NTAPI* PNP_CONTROL_FUNCTION)(
    _In_ ULONG PnPControlClass,
    _In_ PVOID PnPControlData,
    _In_ ULONG PnPControlDataLength,
    _In_ KPROCESSOR_MODE AccessMode
);

typedef struct _PNP_CONTROL_HANDLER
{ 
    ULONG ControlCode;
    ULONG Size;
    PNP_CONTROL_FUNCTION Function;
} PNP_CONTROL_HANDLER, *PPNP_CONTROL_HANDLER; 

typedef struct _PNP_DEVICE_EVENT_LIST {
    NTSTATUS Status;
    KMUTANT EventQueueMutex;
    KGUARDED_MUTEX Lock;
    LIST_ENTRY List;
} PNP_DEVICE_EVENT_LIST, *PPNP_DEVICE_EVENT_LIST;

typedef struct _PNP_DEVICE_EVENT_ENTRY
{
    LIST_ENTRY ListEntry;
    ULONG Argument;
    PKEVENT CallerEvent;
    PVOID Callback;
    PVOID Context;
    PPNP_VETO_TYPE VetoType;
    PUNICODE_STRING VetoName;
    PLUGPLAY_EVENT_BLOCK Data;
} PNP_DEVICE_EVENT_ENTRY, *PPNP_DEVICE_EVENT_ENTRY;

typedef struct _DRIVER_GROUP_LIST_ENTRY {
    struct _DRIVER_GROUP_LIST_ENTRY * ShortEntry;
    struct _DRIVER_GROUP_LIST_ENTRY * LongEntry;
    struct _DRIVER_GROUP_LIST_ENTRY * NextSameEntry;
    ULONG Param;
    ULONG NumberOfLoads;
    UNICODE_STRING GroupName;
    WCHAR Buffer[];
} DRIVER_GROUP_LIST_ENTRY, *PDRIVER_GROUP_LIST_ENTRY;

typedef struct _PNP_REMOVAL_WALK_CONTEXT
{
    BOOLEAN TreeDeletion;
    BOOLEAN DescendantNode;
    UCHAR Pad[2];
} PNP_REMOVAL_WALK_CONTEXT, *PPNP_REMOVAL_WALK_CONTEXT;

typedef struct _CLASS_INFO_BUFFER
{
    PCHAR StartBuffer;
    PCHAR LastBuffer;
    ULONG MaxSize;
} CLASS_INFO_BUFFER, *PCLASS_INFO_BUFFER;

typedef struct _RELATION_LIST_ENTRY
{
    ULONG Count;
    ULONG MaxCount;
    PDEVICE_OBJECT Devices[1];
} RELATION_LIST_ENTRY, *PRELATION_LIST_ENTRY;

typedef struct _RELATION_LIST
{
    ULONG Count;
    ULONG TagCount;
    ULONG FirstLevel;
    ULONG MaxLevel;
    PRELATION_LIST_ENTRY Entries[1];
} RELATION_LIST, *PRELATION_LIST;

typedef enum _IRPLOCK
{
    IRPLOCK_CANCELABLE      = 0,
    IRPLOCK_CANCEL_STARTED  = 1,
    IRPLOCK_CANCEL_COMPLETE = 2,
    IRPLOCK_COMPLETED       = 3
} IRPLOCK, *PIRPLOCK;

typedef struct _DOCK_INTERFACE
{
    UINT16 Size;
    UINT16 Version;
    PVOID Context;
    PVOID InterfaceReference;
    PVOID InterfaceDereference;
    PVOID ProfileDepartureSetMode;
    PVOID ProfileDepartureUpdate;
} DOCK_INTERFACE, *PDOCK_INTERFACE;

typedef struct _PENDING_RELATIONS_LIST_ENTRY
{
    LIST_ENTRY Link;
    WORK_QUEUE_ITEM WorkItem;
    PPNP_DEVICE_EVENT_ENTRY DeviceEvent;
    PDEVICE_OBJECT DeviceObject;
    PRELATION_LIST RelationsList;
    PIRP EjectIrp;
    IRPLOCK Lock;
    ULONG Problem;
    BOOLEAN ProfileChangingEject;
    BOOLEAN DisplaySafeRemovalDialog;
    BOOLEAN Padding[0x2];
    SYSTEM_POWER_STATE LightestSleepState;
    PDOCK_INTERFACE DockInterface;
} PENDING_RELATIONS_LIST_ENTRY, *PPENDING_RELATIONS_LIST_ENTRY;

typedef struct _PNP_NOTIFY_HEADER
{
    LIST_ENTRY Link;
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    ULONG SessionId;
    PVOID OpaqueSession;
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE  PnpNotificationRoutine;
    PVOID Context;
    PDRIVER_OBJECT DriverObject;
    USHORT RefCount;
    BOOLEAN Unregistered;
    UCHAR Padded;
    PKGUARDED_MUTEX NotifyLock;
} PNP_NOTIFY_HEADER, *PPNP_NOTIFY_HEADER;

typedef struct _HARDWARE_PROFILE_NOTIFY
{
    PNP_NOTIFY_HEADER Header;
} HARDWARE_PROFILE_NOTIFY, *PHARDWARE_PROFILE_NOTIFY; 

typedef struct _DEVICE_INTERFACE_NOTIFY
{
    PNP_NOTIFY_HEADER Header;
    GUID Interface;
} DEVICE_INTERFACE_NOTIFY, *PDEVICE_INTERFACE_NOTIFY; 

typedef struct _TARGET_DEVICE_NOTIFY
{
    PNP_NOTIFY_HEADER Header;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT PhysicalDeviceObject;
} TARGET_DEVICE_NOTIFY, *PTARGET_DEVICE_NOTIFY; 

typedef struct _SETUP_NOTIFY_DATA
{
    PNP_NOTIFY_HEADER Header;
} SETUP_NOTIFY_DATA, *PSETUP_NOTIFY_DATA; 

typedef struct _PNP_DEFER_NOTIFY
{
    LIST_ENTRY Link;
    PPNP_NOTIFY_HEADER NotifyHeader;
} PNP_DEFER_NOTIFY, *PPNP_DEFER_NOTIFY; 

/* === IO mgr ================================ */

/* device.c */
NTSTATUS
NTAPI
IopGetRelatedTargetDevice(
    _In_ PFILE_OBJECT FileObject,
    _Out_ PDEVICE_NODE* DeviceNode
);

/* deviface.c */
VOID
NTAPI
IopDoDeferredSetInterfaceState(
    _In_ PDEVICE_NODE DeviceNode
);

NTSTATUS
NTAPI
IopDisableDeviceInterfaces(
    _In_ PUNICODE_STRING InstancePath
);

/* driver.c */
NTSTATUS
NTAPI
IopInvalidDeviceRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
);

VOID
FASTCALL
IopDisplayLoadingMessage(
    _In_ PUNICODE_STRING ServiceName
);

BOOLEAN
NTAPI
IopIsLegacyDriver(
    _In_ PDRIVER_OBJECT DriverObject
);

NTSTATUS
NTAPI
IopLoadDriver(
    _In_ HANDLE ServiceHandle,
    _In_ BOOLEAN SafeBootModeFlag,
    _In_ BOOLEAN IsFilter,
    _Out_ NTSTATUS * OutInitStatus
);

VOID
NTAPI
IopLoadUnloadDriver(
    _In_ PVOID Context
);

NTSTATUS
NTAPI
IopUnloadDriver(
    _In_ PUNICODE_STRING DriverServiceName,
    _In_ BOOLEAN IsUnloadPnpManagers
);

/* volume.c */
NTSTATUS
NTAPI
IopInvalidateVolumesForDevice(
    _In_ PDEVICE_OBJECT DeviceObject
);

ULONG
FASTCALL
IopInterlockedIncrementUlong(
    _In_ KSPIN_LOCK_QUEUE_NUMBER Queue,
    _Inout_ PULONG Ulong
);

VOID
NTAPI
IopDecrementDeviceObjectRef(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN UnloadIfUnused,
    _In_ BOOLEAN IsDelayedWorker
);

VOID
NTAPI
IopDecrementDeviceObjectHandleCount(
    _Inout_ PDEVICE_OBJECT FileDeviceObject
);

/* === PNP mgr =============================== */

/* debug.c */
VOID
NTAPI
PipDumpCmResourceList(
    _In_ PCM_RESOURCE_LIST CmResource,
    _In_ ULONG DebugLevel
);

PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
PipGetNextCmPartialDescriptor(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor
);

VOID
NTAPI
PipDumpCmResourceDescriptor(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ ULONG DebugLevel
);

VOID
NTAPI
PipDumpResourceRequirementsList(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST IoResource,
    _In_ ULONG DebugLevel
);

VOID
NTAPI
PipDumpIoResourceDescriptor(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ ULONG DebugLevel
);

VOID
NTAPI
PipDumpDeviceNodes(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG Flags,
    _In_ ULONG DebugLevel
);

PWCHAR
NTAPI
IopGetBusName(
    _In_ INTERFACE_TYPE IfType
);

VOID
NTAPI
IopDumpDeviceNode(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG Flags,
    _In_ PUNICODE_STRING ServiceName
);

VOID
NTAPI
IopDumpReqDescriptor(
    _In_ PPNP_REQ_DESCRIPTOR Descriptor,
    _In_ ULONG Idx
);

VOID
NTAPI
IopDumpResRequest(
    _In_ PPNP_RESOURCE_REQUEST ResRequest
);

/* plugplay.c */
NTSTATUS NTAPI PiControlEnumerateDevice          (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlRegisterNewDevice        (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlDeregisterDevice         (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlInitializeDevice         (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlStartDevice              (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlQueryAndRemoveDevice     (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlUserResponse             (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlGenerateLegacyDevice     (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlGetInterfaceDeviceList   (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlGetPropertyData          (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlDeviceClassAssociation   (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlGetRelatedDevice         (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlGetInterfaceDeviceAlias  (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlGetSetDeviceStatus       (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlGetDeviceDepth           (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlQueryDeviceRelations     (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlQueryTargetDeviceRelation(ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlQueryConflictList        (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlRetrieveDockData         (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlResetDevice              (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlHaltDevice               (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);
NTSTATUS NTAPI PiControlGetBlockedDriverData     (ULONG PnPControlClass, PVOID PnPControlData, ULONG PnPControlDataLength, KPROCESSOR_MODE AccessMode);

NTSTATUS
NTAPI
PiControlMakeUserModeCallersCopy(
    _Out_ PVOID* pOutBuffer,
    _In_ PVOID Buffer,
    _In_ SIZE_T Size,
    _In_ ULONG Alignment,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN IsReadFromBuffer
);

/* pnpdel.c */
BOOLEAN
NTAPI
IopEnumerateRelations(
    _In_ PRELATION_LIST RelationList,
    _In_ PULONG Marker,
    _Out_ PDEVICE_OBJECT * OutEnumDevice,
    _Out_ PUCHAR OutIsDirectDescendant,
    _Out_ PBOOLEAN OutIsTagged,
    _In_ BOOLEAN Direction
);

NTSTATUS
NTAPI
IopProcessRelation(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PIP_TYPE_REMOVAL_DEVICE RemovalType,
    _In_ BOOLEAN IsDirectDescendant,
    _In_ PPNP_VETO_TYPE VetoType,
    _In_ PUNICODE_STRING VetoName,
    _In_ PRELATION_LIST RelationsList
);

NTSTATUS
NTAPI
IopBuildRemovalRelationList(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIP_TYPE_REMOVAL_DEVICE RemovalType,
    _In_ PPNP_VETO_TYPE VetoType,
    _In_ PUNICODE_STRING VetoName,
    _Out_ PRELATION_LIST * OutRelationList
);

VOID
NTAPI
PipRequestDeviceRemoval(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR TreeDeletion,
    _In_ ULONG Problem
);

NTSTATUS
NTAPI
IopDeleteLockedDeviceNodes(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PRELATION_LIST RelationsList,
    _In_ PIP_TYPE_REMOVAL_DEVICE RemovalType,
    _In_ BOOLEAN IsForceDescendant,
    _In_ ULONG Problem,
    _In_ PPNP_VETO_TYPE VetoType,
    _In_ PUNICODE_STRING VetoName
);

VOID
NTAPI
IopFreeRelationList(
    _In_ PRELATION_LIST RelationsList
);

VOID
NTAPI
PipRequestDeviceRemoval(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ UCHAR TreeDeletion,
    _In_ ULONG Problem
);

VOID
NTAPI
IopInvalidateRelationsInList(
    _In_ PRELATION_LIST RelationsList,
    _In_ PIP_TYPE_REMOVAL_DEVICE RemovalType,
    _In_ BOOLEAN Param3,
    _In_ BOOLEAN Param4
);

VOID
NTAPI
IopUnlinkDeviceRemovalRelations(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PRELATION_LIST RelationsList,
    _In_ ULONG Type);

VOID
NTAPI
IopQueuePendingSurpriseRemoval(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PRELATION_LIST RelationsList,
    _In_ ULONG Problem
);

BOOLEAN
NTAPI
IopNotifyPnpWhenChainDereferenced(
    _In_ PDEVICE_OBJECT* RemovalDevices,
    _In_ ULONG DevicesCount,
    _In_ BOOLEAN IsQueryRemove,
    _In_ BOOLEAN IsAllowRunWorker,
    _Out_ PDEVICE_OBJECT* OutDeviceObject
);

/* pnpenum.c */
NTSTATUS
NTAPI
PpCriticalProcessCriticalDevice(
    _In_ PDEVICE_NODE DeviceNode
);

NTSTATUS
NTAPI
IopQueryAndSaveDeviceNodeCapabilities(
    _In_ PDEVICE_NODE DeviceNode
);

NTSTATUS
NTAPI
PipRequestDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIP_ENUM_TYPE RequestType,
    _In_ UCHAR ReorderingBarrier,
    _In_ ULONG_PTR RequestArgument,
    _In_ PKEVENT CompletionEvent,
    _Inout_ NTSTATUS * CompletionStatus
);

VOID
NTAPI
PpMarkDeviceStackStartPending(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN MarkType
);

NTSTATUS
NTAPI
PipCallDriverAddDevice(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN IsLoadDriver,
    _In_ SERVICE_LOAD_TYPE * DriverLoadType
);

VOID
NTAPI
IopUncacheInterfaceInformation(
    _In_ PDEVICE_OBJECT DeviceObject
);

PDRIVER_OBJECT
NTAPI
IopReferenceDriverObjectByName(
    _In_ PUNICODE_STRING Name
);

NTSTATUS
NTAPI
IopRestartDeviceNode(
    _In_ PDEVICE_NODE DeviceNode
);

/* pnpevent.c */
NTSTATUS
NTAPI
PpSynchronizeDeviceEventQueue(
    VOID
);

NTSTATUS
NTAPI
PpSetDeviceClassChange(
    _In_ CONST GUID * EventGuid,
    _In_ GUID * ClassGuid,
    _In_ PUNICODE_STRING SymbolicLinkName
);

PVOID
NTAPI
PiAllocateCriticalMemory(
    _In_ PIP_TYPE_REMOVAL_DEVICE RemoveType,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag
);

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
    _Out_ NTSTATUS * OutResult,
    _In_ PPNP_VETO_TYPE VetoType,
    _In_ PUNICODE_STRING VetoName
);

BOOLEAN
NTAPI
IopNotifyPnpWhenChainDereferenced(
    _In_ PDEVICE_OBJECT * RemovalDevices,
    _In_ ULONG DevicesCount,
    _In_ BOOLEAN IsQueryRemove,
    _In_ BOOLEAN IsAllowRunWorker,
    _Out_ PDEVICE_OBJECT * OutDeviceObject
);

/* pnpinit.c */
CODE_SEG("INIT")
BOOLEAN
FASTCALL
IopInitializeBootDrivers(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock
);

CODE_SEG("INIT")
VOID
FASTCALL
IopInitializeSystemDrivers(
    VOID
);

NTSTATUS
NTAPI
IopPrepareDriverLoading(
    _In_ PUNICODE_STRING ServiceKeyName,
    _In_ HANDLE ServiceKeyHandle,
    _In_ PVOID ImageBase,
    _In_ BOOLEAN IsFilter
);

NTSTATUS
NTAPI
IopInitializeBuiltinDriver(
    _In_ PUNICODE_STRING DriverName,
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PDRIVER_INITIALIZE EntryPoint,
    _In_ PLDR_DATA_TABLE_ENTRY BootLdrEntry,
    _In_ BOOLEAN IsFilter,
    _Out_ PDRIVER_OBJECT * OutDriverObject
);

/* pnpirp.c */
NTSTATUS
NTAPI
IopQueryDeviceRelations(
    _In_ DEVICE_RELATION_TYPE RelationsType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PDEVICE_RELATIONS * OutPendingDeviceRelations
);

NTSTATUS
NTAPI
PpIrpQueryID(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BUS_QUERY_ID_TYPE IdType,
    _Out_ PWCHAR *OutID
);

NTSTATUS
NTAPI
PpIrpQueryCapabilities(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PDEVICE_CAPABILITIES DeviceCapabilities
);

NTSTATUS
NTAPI
IopQueryDeviceState(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PNP_DEVICE_STATE *OutState
);

NTSTATUS
NTAPI
PpIrpQueryDeviceText(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_TEXT_TYPE DeviceTextType,
    _In_ LCID LocaleId,
    _Out_ PWCHAR * OutDeviceText
);

NTSTATUS
NTAPI
PpIrpQueryResourceRequirements(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST * IoResource
);

NTSTATUS
NTAPI
PpIrpQueryBusInformation(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PPNP_BUS_INFORMATION * OutInformation
);

NTSTATUS
NTAPI
PpIrpQueryResources(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PCM_RESOURCE_LIST * OutResourceList,
    _Out_ PULONG OutSize
);

NTSTATUS
NTAPI
IopQueryLegacyBusInformation(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ GUID * OutBusTypeGuid,
    _Out_ INTERFACE_TYPE * OutInterfaceType,
    _Out_ PULONG OutBusNumber
);

NTSTATUS
NTAPI
IopQueryResourceHandlerInterface(
    _In_ ULONG InterfaceType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR InterfaceSpecificData,
    _Out_ PVOID * OutInterface
);

NTSTATUS
NTAPI
IopFilterResourceRequirementsCall(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST * OutRequirementsList
);

NTSTATUS
NTAPI
IopStartDevice(
    _In_ PDEVICE_NODE DeviceNode
);

NTSTATUS
NTAPI
IopQueryReconfiguration(
    _In_ UCHAR MinorFunction,
    _In_ PDEVICE_OBJECT DeviceObject
);

NTSTATUS
NTAPI
IopRemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorCode
);

NTSTATUS
NTAPI
PiIrpQueryRemoveDevice(
    _In_ PDEVICE_OBJECT TargetDevice,
    _Out_ PDRIVER_OBJECT * OutFailDriverObject
);

/* pnpmap.c */
VOID
NTAPI
MapperFreeList(
    VOID
);

VOID
NTAPI
MapperConstructRootEnumTree(
    _In_ BOOLEAN IsDisableMapper
);

NTSTATUS
NTAPI
MapperProcessFirmwareTree(
    _In_ BOOLEAN IsDisableMapper
);

/* pnpmgr.c */
USHORT
NTAPI
IopGetBusTypeGuidIndex(
    _In_ LPGUID BusTypeGuid
);

NTSTATUS
NTAPI
PiGetDeviceRegistryProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ValueType,
    IN PWSTR ValueName,
    IN PWSTR KeyName,
    OUT PVOID Buffer,
    IN PULONG BufferLength
);

NTSTATUS
NTAPI
PnpDeviceObjectToDeviceInstance(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PHANDLE DeviceInstanceHandle,
    _In_ ACCESS_MASK DesiredAccess
);

NTSTATUS
NTAPI
PpDeviceRegistration(
    _In_ PUNICODE_STRING InstancePath,
    _In_ BOOLEAN Param1,
    _In_ PUNICODE_STRING ServiceName
);

/* pnpnode.c */
VOID
NTAPI
PipSetDevNodeState(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PNP_DEVNODE_STATE NewState,
    _Out_ PNP_DEVNODE_STATE *OutPreviousState
);

VOID
NTAPI
PipRestoreDevNodeState(
    _In_ PDEVICE_NODE DeviceNode
);

VOID
NTAPI
PipSetDevNodeProblem(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG Problem
);

VOID
NTAPI
PipClearDevNodeProblem(
    _In_ PDEVICE_NODE DeviceNode
);

VOID
NTAPI
PpDevNodeInsertIntoTree(
    _In_ PDEVICE_NODE ParentNode,
    _In_ PDEVICE_NODE DeviceNode
);

VOID
NTAPI
PpHotSwapUpdateRemovalPolicy(
    _In_ PDEVICE_NODE DeviceNode
);

VOID
NTAPI
PpHotSwapGetDevnodeRemovalPolicy(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN Type,
    _Out_ PDEVICE_REMOVAL_POLICY OutPolicy
);

VOID
NTAPI
PpHotSwapInitRemovalPolicy(
    _In_ PDEVICE_NODE DeviceNode
);

VOID
NTAPI
IopInsertLegacyBusDeviceNode(
    _In_ PDEVICE_NODE LegacyDeviceNode,
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG Bus
);

BOOLEAN
NTAPI
PipIsDevNodeDNStarted(
    _In_ PDEVICE_NODE DeviceNode
);

BOOLEAN
NTAPI
PipAreDriversLoaded(
    _In_ PDEVICE_NODE DeviceNode
);

VOID
NTAPI
IopDestroyDeviceNode(
    _In_ PDEVICE_NODE DeviceNode
);

VOID
NTAPI
IopMarkHalDeviceNode(
    VOID
);

VOID
NTAPI
PpDevNodeAssertLockLevel(
    _In_ LONG Level
);

BOOLEAN
NTAPI
PipIsProblemReadonly(
    _In_ ULONG Problem
);

VOID
NTAPI
PpDevNodeRemoveFromTree(
    _In_ PDEVICE_NODE DeviceNode
);

/* pnpnotify.c */
VOID
NTAPI
IopProcessDeferredRegistrations(
    VOID
);

VOID
NTAPI
IopNotifyDeviceClassChange(
    _In_ PPLUGPLAY_EVENT_BLOCK EventBlock,
    _In_ GUID * ClassGuid,
    _In_ PUNICODE_STRING SymbolicLinkName
);

NTSTATUS
NTAPI
IopNotifyTargetDeviceChange(
    _In_ GUID* EventGuid,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PTARGET_DEVICE_CUSTOM_NOTIFICATION CustomStructure,
    _Out_ PDRIVER_OBJECT* OutDriverObject
);

/* pnpres.c */
NTSTATUS
NTAPI
IopWriteResourceList(
    _In_ HANDLE ResourceHandle,
    _In_ PUNICODE_STRING ResourceName,
    _In_ PUNICODE_STRING Description,
    _In_ PUNICODE_STRING ValueName,
    _In_ PCM_RESOURCE_LIST CmResource,
    _In_ ULONG ListSize
);

BOOLEAN
NTAPI
IopProcessAssignResources(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN IncludeFailedDevices,
    _Inout_ BOOLEAN *OutIsAssigned
);

NTSTATUS
NTAPI
IopGetDeviceResourcesFromRegistry(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BOOLEAN ResourcesType,
    _In_ ULONG VectorType,
    _Out_ PVOID * OutResource,
    _Out_ SIZE_T * OutSize
);

NTSTATUS
NTAPI
IopAllocateBootResources(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST CmResource
);

NTSTATUS
NTAPI
IopReportBootResources(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST CmResource
);

VOID
NTAPI
IopAllocateLegacyBootResources(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber
);

NTSTATUS
NTAPI
IopAllocateBootResourcesInternal(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCM_RESOURCE_LIST CmResource
);

NTSTATUS
NTAPI
IopReleaseDeviceResources(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN IsAllocateBootResources
);

VOID
NTAPI
IopAllocateLegacyBootResources(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber
);

VOID
NTAPI
IopReallocateResources(
    _In_ PDEVICE_NODE DeviceNode
);

NTSTATUS
NTAPI
IopWriteAllocatedResourcesToRegistry(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PCM_RESOURCE_LIST CmResource,
    _In_ ULONG DataSize
);

NTSTATUS
NTAPI 
IopLegacyResourceAllocation(
    _In_ ARBITER_REQUEST_SOURCE AllocationType,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT DeviceObject OPTIONAL,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    _Inout_ PCM_RESOURCE_LIST * AllocatedResources
);

PDEVICE_NODE
NTAPI
IopFindLegacyBusDeviceNode(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG LegacyBusNumber
);

/* pnputil.c */
BOOLEAN
NTAPI
PiCompareGuid(
    _In_ CONST GUID* Guid1,
    _In_ CONST GUID* Guid2
);

NTSTATUS
NTAPI
PnpAllocateUnicodeString(
    _Out_ PUNICODE_STRING String,
    _In_ USHORT Size
);

NTSTATUS
NTAPI
PnpConcatenateUnicodeStrings(
    _Out_ PUNICODE_STRING DestinationString,
    _In_ PUNICODE_STRING SourceString,
    _In_ PUNICODE_STRING AppendString
);

VOID
NTAPI
PnpFreeUnicodeStringList(
    _In_ PUNICODE_STRING UnicodeStringList,
    _In_ ULONG StringCount
);

NTSTATUS
NTAPI
IopMapDeviceObjectToDeviceInstance(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PUNICODE_STRING InstancePath
);

NTSTATUS
NTAPI
IopCleanupDeviceRegistryValues(
    _In_ PUNICODE_STRING InstancePath
);

PDEVICE_OBJECT
NTAPI
IopDeviceObjectFromDeviceInstance(
    _In_ PUNICODE_STRING DeviceInstance
);

NTSTATUS
NTAPI
PipApplyFunctionToSubKeys(
    _In_opt_ HANDLE RootHandle,
    _In_opt_ PUNICODE_STRING KeyName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ UCHAR Flags,
    _In_ PIP_FUNCTION_TO_SUBKEYS Function,
    _In_ PVOID Context
);

NTSTATUS
NTAPI
IopGetDeviceInstanceCsConfigFlags(
    _In_ PUNICODE_STRING InstanceName,
    _Out_ PULONG OutConfigFlagsValue
);

NTSTATUS
NTAPI
PipOpenServiceEnumKeys(
    _In_ PUNICODE_STRING ServiceString,
    _In_ ACCESS_MASK Aaccess,
    _Out_ PHANDLE OutHandle,
    _Out_ PHANDLE OutEnumHandle,
    _In_ BOOLEAN IsCreate
);

NTSTATUS
NTAPI
IopOpenDeviceParametersSubkey(
    _Out_ PHANDLE OutHandle,
    _In_opt_ HANDLE ParentKey,
    _In_ PUNICODE_STRING NameString,
    _In_ ACCESS_MASK Access
);

NTSTATUS
NTAPI
IopReplaceSeparatorWithPound(
    _Out_ PUNICODE_STRING OutString,
    _In_ PUNICODE_STRING InString
);

BOOLEAN
NTAPI
IopIsDeviceInstanceEnabled(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING Instance,
    _In_ BOOLEAN IsDisableDevice
);

NTSTATUS
NTAPI
IopGetDriverNameFromKeyNode(
    _In_ HANDLE KeyHandle,
    _Inout_ PUNICODE_STRING OutDriverName
);

BOOLEAN
NTAPI
IopIsAnyDeviceInstanceEnabled(
    _In_ PUNICODE_STRING ServiceKeyName,
    _In_ HANDLE ServiceKeyHandle,
    _In_ BOOLEAN IsLegacyDriver
);

PIO_RESOURCE_REQUIREMENTS_LIST
NTAPI
IopCmResourcesToIoResources(
    _In_ ULONG Slot,
    _In_ PCM_RESOURCE_LIST CmResource,
    _In_ ULONG Priority
);

NTSTATUS
NTAPI
IopOpenRegistryKey(
    _Out_ PHANDLE KeyHandle,
    _In_ HANDLE RootKeyHandle,
    _In_ PUNICODE_STRING KeyName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN IsCreateOrOpen
);

NTSTATUS
NTAPI
IopOpenRegistryKeyEx(
    _Out_ PHANDLE KeyHandle,
    _In_ HANDLE RootKeyHandle,
    _In_ PUNICODE_STRING KeyName,
    _In_ ACCESS_MASK DesiredAccess
);

ULONG
NTAPI
PiFixupID(
    _In_ PWCHAR Id,
    _In_ ULONG MaxIdLen,
    _In_ BOOLEAN IsMultiSz,
    _In_ ULONG MaxSeparators,
    _In_ PUNICODE_STRING ServiceName
);

VOID
NTAPI
IopDeleteLegacyKey(
    _In_ PDRIVER_OBJECT DriverObject
);

NTSTATUS
NTAPI
PipGenerateMadeupNodeName(
    _In_ PUNICODE_STRING ServiceKeyName,
    _Out_ PUNICODE_STRING OutMadeupName
);

NTSTATUS
NTAPI
PipServiceInstanceToDeviceInstance(
    _In_ HANDLE ServiceKeyHandle,
    _In_ PUNICODE_STRING ServiceKeyName,
    _In_ ULONG InstanceNum,
    _Out_ PUNICODE_STRING OutInstanceName,
    _Out_ PHANDLE OutHandle,
    _In_ ACCESS_MASK DesiredAccess
);

NTSTATUS
NTAPI
IopDriverLoadingFailed(
    _In_ HANDLE ServiceKeyHandle,
    _In_ PUNICODE_STRING ServiceKeyName
);

#endif  /* _PNPIO_H */

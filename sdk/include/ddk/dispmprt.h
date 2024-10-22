/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header file for WDDM style driver exports
 * COPYRIGHT:   Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */


#ifndef _DISPMPRT_H_
#define _DISPMPRT_H_

/* DxgKrnl can enumerate ACPI. */
#ifndef _ACPIIOCT_H_
#include <acpiioct.h>
#endif

#define _NTOSDEF_

#ifndef _NTOSP_
#define _NTOSP_

#define DXGK_MAX_STRING_LEN 50
#define DXGK_MAX_REG_SZ_LEN DXGK_MAX_STRING_LEN + 1
#define DXGK_WHICHSPACE_CONFIG       PCI_WHICHSPACE_CONFIG
#define DXGK_WHICHSPACE_ROM          PCI_WHICHSPACE_ROM
#define DXGK_WHICHSPACE_MCH          0x80000000
#define DXGK_WHICHSPACE_BRIDGE       0x80000001


/* Compatiblity */
typedef enum _EMULATOR_PORT_ACCESS_TYPE {
    Uchar,
    Ushort,
    Ulong
} EMULATOR_PORT_ACCESS_TYPE, *PEMULATOR_PORT_ACCESS_TYPE;

typedef struct _EMULATOR_ACCESS_ENTRY {
    ULONG BasePort;
    ULONG NumConsecutivePorts;
    EMULATOR_PORT_ACCESS_TYPE AccessType;
    UCHAR AccessMode;
    UCHAR StringSupport;
    PVOID Routine;
} EMULATOR_ACCESS_ENTRY, *PEMULATOR_ACCESS_ENTRY;

#endif

typedef
VOID
(*PBANKED_SECTION_ROUTINE) (
    _In_ ULONG ReadBank,
    _In_ ULONG WriteBank,
    _In_ PVOID Context
    );

#include <ntddvdeo.h>
#include <video.h>

#include <windef.h>
#include <d3dkmddi.h>
#include <d3dkmdt.h>

typedef enum _DXGK_CHILD_DEVICE_TYPE {
   TypeUninitialized,
   TypeVideoOutput,
   TypeOther,
   TypeIntegratedDisplay
} DXGK_CHILD_DEVICE_TYPE, *PDXGK_CHILD_DEVICE_TYPE;


typedef enum {
    DockStateUnsupported = 0,
    DockStateUnDocked    = 1,
    DockStateDocked      = 2,
    DockStateUnknown     = 3,
} DOCKING_STATE;

typedef enum _DXGK_CHILD_STATUS_TYPE{
   StatusUninitialized,
   StatusConnection,
   StatusRotation,
   StatusMiracastConnection,
} DXGK_CHILD_STATUS_TYPE, *PDXGK_CHILD_STATUS_TYPE;

typedef enum
{
    DxgkServicesAgp,
    DxgkServicesDebugReport,
    DxgkServicesTimedOperation,
    DxgkServicesSPB,
    DxgkServicesBDD,
    DxgkServicesFirmwareTable,
    DxgkServicesIDD,
} DXGK_SERVICES;

typedef struct _LINKED_DEVICE {
    ULONG ChainUid;
    ULONG NumberOfLinksInChain;
    BOOLEAN LeadLink;
} LINKED_DEVICE, *PLINKED_DEVICE;

typedef enum _DXGK_EVENT_TYPE {
    DxgkUndefinedEvent,
    DxgkAcpiEvent,
    DxgkPowerStateEvent,
    DxgkDockingEvent,
    DxgkChainedAcpiEvent
} DXGK_EVENT_TYPE, *PDXGK_EVENT_TYPE;

typedef struct _DXGK_VIDEO_OUTPUT_CAPABILITIES {
    D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY InterfaceTechnology;
    D3DKMDT_MONITOR_ORIENTATION_AWARENESS MonitorOrientationAwareness;
    BOOLEAN SupportsSdtvModes;
} DXGK_VIDEO_OUTPUT_CAPABILITIES, *PDXGK_VIDEO_OUTPUT_CAPABILITIES;

typedef struct _DXGK_INTEGRATED_DISPLAY_CHILD {
    D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY InterfaceTechnology;
    USHORT                          DescriptorLength;
} DXGK_INTEGRATED_DISPLAY_CHILD, *PDXGK_INTEGRATED_DISPLAY_CHILD;

typedef struct _DXGK_CHILD_CAPABILITIES {

    union
    {
        DXGK_VIDEO_OUTPUT_CAPABILITIES  VideoOutput;

        struct
        {
            UINT MustBeZero;
        }
        Other;

        DXGK_INTEGRATED_DISPLAY_CHILD   IntegratedDisplayChild;
    } Type;

    DXGK_CHILD_DEVICE_HPD_AWARENESS HpdAwareness;
} DXGK_CHILD_CAPABILITIES, *PDXGK_CHILD_CAPABILITIES;

typedef struct _DXGK_CHILD_DESCRIPTOR {
   DXGK_CHILD_DEVICE_TYPE ChildDeviceType;
   DXGK_CHILD_CAPABILITIES ChildCapabilities;
   ULONG AcpiUid;
   ULONG ChildUid;
} DXGK_CHILD_DESCRIPTOR, *PDXGK_CHILD_DESCRIPTOR;

typedef struct _DXGK_CHILD_STATUS {
   DXGK_CHILD_STATUS_TYPE Type;
   ULONG ChildUid;
   union {
      struct {
         BOOLEAN Connected;
      } HotPlug;
      struct {
         UCHAR Angle;
      } Rotation;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
      struct {
         BOOLEAN Connected;
         D3DKMDT_VIDEO_OUTPUT_TECHNOLOGY MiracastMonitorType;
      } Miracast;
#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
   };
} DXGK_CHILD_STATUS, *PDXGK_CHILD_STATUS;

typedef struct _DXGK_DEVICE_INFO {
    PVOID MiniportDeviceContext;
    PDEVICE_OBJECT PhysicalDeviceObject;
    UNICODE_STRING DeviceRegistryPath;
    PCM_RESOURCE_LIST TranslatedResourceList;
    LARGE_INTEGER SystemMemorySize;
    PHYSICAL_ADDRESS HighestPhysicalAddress;
    PHYSICAL_ADDRESS AgpApertureBase;
    SIZE_T AgpApertureSize;
    DOCKING_STATE DockingState;
} DXGK_DEVICE_INFO, *PDXGK_DEVICE_INFO;

typedef struct _DXGK_DEVICE_DESCRIPTOR {
   ULONG DescriptorOffset;
   ULONG DescriptorLength;
   PVOID DescriptorBuffer;
} DXGK_DEVICE_DESCRIPTOR, *PDXGK_DEVICE_DESCRIPTOR;

/*
 * Passed from DxgKrnl -> Miniport
 * Implemented by enumeration of the device.
 */
typedef struct _DXGK_START_INFO {
    ULONG                          RequiredDmaQueueEntry;
    GUID                           AdapterGuid;
#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    LUID                           AdapterLuid;
#endif // DXGKDDI_INTERFACE_VERSION_WIN8
} DXGK_START_INFO, *PDXGK_START_INFO;

typedef
VOID
(*DXGKDDI_PROTECTED_CALLBACK)(
    _In_ const PVOID MiniportDeviceContext,
    _In_ PVOID ProtectedCallbackContext,
    _In_ NTSTATUS ProtectionStatus);

typedef
NTSTATUS
(APIENTRY *DXGKCB_EVAL_ACPI_METHOD)(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG DeviceUid,
    _In_reads_bytes_(AcpiInputSize) PACPI_EVAL_INPUT_BUFFER_COMPLEX AcpiInputBuffer,
    _In_range_(>=, sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX)) ULONG AcpiInputSize,
    _Out_writes_bytes_(AcpiOutputSize) PACPI_EVAL_OUTPUT_BUFFER AcpiOutputBuffer,
    _In_range_(>=, sizeof(ACPI_EVAL_OUTPUT_BUFFER)) ULONG AcpiOutputSize);

typedef
NTSTATUS
(APIENTRY *DXGKCB_GET_DEVICE_INFORMATION)(
    _In_  HANDLE DeviceHandle,
    _Out_ PDXGK_DEVICE_INFO DeviceInfo);

typedef
NTSTATUS
(APIENTRY *DXGKCB_INDICATE_CHILD_STATUS)(
    _In_ HANDLE DeviceHandle,
    _In_ PDXGK_CHILD_STATUS ChildStatus);

typedef
NTSTATUS
(APIENTRY *DXGKCB_MAP_MEMORY)(
    _In_ HANDLE DeviceHandle,
    _In_ PHYSICAL_ADDRESS TranslatedAddress,
    _In_ ULONG Length,
    _In_ BOOLEAN InIoSpace,
    _In_ BOOLEAN MapToUserMode,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _Outptr_ PVOID *VirtualAddress);

typedef
NTSTATUS
(APIENTRY *DXGKCB_QUERY_SERVICES)(
    _In_ HANDLE DeviceHandle,
    _In_ DXGK_SERVICES ServicesType,
    _Inout_ PINTERFACE Interface);

typedef
BOOLEAN
(APIENTRY *DXGKCB_QUEUE_DPC)(
    _In_ HANDLE DeviceHandle);

typedef
NTSTATUS
(APIENTRY *DXGKCB_READ_DEVICE_SPACE)(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG DataType,
    _Out_writes_bytes_to_(Length, *BytesRead) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Out_ PULONG BytesRead);

typedef
NTSTATUS
(APIENTRY *DXGKCB_SYNCHRONIZE_EXECUTION)(
    _In_ HANDLE DeviceHandle,
    _In_ PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    _In_ PVOID Context,
    _In_ ULONG MessageNumber,
    _Out_ PBOOLEAN ReturnValue);

typedef
NTSTATUS
(APIENTRY *DXGKCB_UNMAP_MEMORY)(
    _In_ HANDLE DeviceHandle,
    _In_ PVOID VirtualAddress);

typedef
NTSTATUS
(APIENTRY *DXGKCB_WRITE_DEVICE_SPACE)(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG DataType,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _Out_ _Out_range_(<=, Length) PULONG BytesWritten);

typedef
NTSTATUS
(APIENTRY *DXGKCB_IS_DEVICE_PRESENT)(
    _In_ HANDLE DeviceHandle,
    _In_ PPCI_DEVICE_PRESENCE_PARAMETERS DevicePresenceParameters,
    _Out_ PBOOLEAN DevicePresent);

typedef
VOID
(APIENTRY *DXGKCB_LOG_ETW_EVENT)(
    _In_ CONST LPCGUID EventGuid,
    _In_ UCHAR Type,
    _In_ USHORT EventBufferSize,
    _In_reads_bytes_(EventBufferSize) PVOID EventBuffer);

typedef
NTSTATUS
(APIENTRY *DXGKCB_EXCLUDE_ADAPTER_ACCESS)(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG Attributes,
    _In_ DXGKDDI_PROTECTED_CALLBACK DxgkProtectedCallback,
    _In_ PVOID ProtectedCallbackContext);

/*
 * Passed from DxgKrnl -> Miniport
 * Implemented by DxgKrnl itself.
 */
typedef struct _DXGKRNL_INTERFACE {
    ULONG                                   Size;
    ULONG                                   Version;
    HANDLE                                  DeviceHandle;
    DXGKCB_EVAL_ACPI_METHOD                 DxgkCbEvalAcpiMethod;
    DXGKCB_GET_DEVICE_INFORMATION           DxgkCbGetDeviceInformation;
    DXGKCB_INDICATE_CHILD_STATUS            DxgkCbIndicateChildStatus;
    DXGKCB_MAP_MEMORY                       DxgkCbMapMemory;
    DXGKCB_QUEUE_DPC                        DxgkCbQueueDpc;
    DXGKCB_QUERY_SERVICES                   DxgkCbQueryServices;
    DXGKCB_READ_DEVICE_SPACE                DxgkCbReadDeviceSpace;
    DXGKCB_SYNCHRONIZE_EXECUTION            DxgkCbSynchronizeExecution;
    DXGKCB_UNMAP_MEMORY                     DxgkCbUnmapMemory;
    DXGKCB_WRITE_DEVICE_SPACE               DxgkCbWriteDeviceSpace;
    DXGKCB_IS_DEVICE_PRESENT                DxgkCbIsDevicePresent;
    DXGKCB_GETHANDLEDATA                    DxgkCbGetHandleData;
    DXGKCB_GETHANDLEPARENT                  DxgkCbGetHandleParent;
    DXGKCB_ENUMHANDLECHILDREN               DxgkCbEnumHandleChildren;
    DXGKCB_NOTIFY_INTERRUPT                 DxgkCbNotifyInterrupt;
    DXGKCB_NOTIFY_DPC                       DxgkCbNotifyDpc;
    DXGKCB_QUERYVIDPNINTERFACE              DxgkCbQueryVidPnInterface;
    DXGKCB_QUERYMONITORINTERFACE            DxgkCbQueryMonitorInterface;
    DXGKCB_GETCAPTUREADDRESS                DxgkCbGetCaptureAddress;
    DXGKCB_LOG_ETW_EVENT                    DxgkCbLogEtwEvent;
    DXGKCB_EXCLUDE_ADAPTER_ACCESS           DxgkCbExcludeAdapterAccess;
    // Vista / 7
} DXGKRNL_INTERFACE, *PDXGKRNL_INTERFACE;


typedef
NTSTATUS
NTAPI
DXGKDDI_ADD_DEVICE(_In_  const PDEVICE_OBJECT     PhysicalDeviceObject,
                   _Out_ PVOID*                   MiniportDeviceContext);

typedef DXGKDDI_ADD_DEVICE                      *PDXGKDDI_ADD_DEVICE;


typedef
NTSTATUS
NTAPI
DXGKDDI_START_DEVICE(_In_  PVOID                MiniportDeviceContext,
                     _In_  PDXGK_START_INFO     DxgkStartInfo,
                     _In_  PDXGKRNL_INTERFACE   DxgkInterface,
                     _Out_ PULONG               NumberOfVideoPresentSources,
                     _Out_ PULONG               NumberOfChildren);

typedef DXGKDDI_START_DEVICE                    *PDXGKDDI_START_DEVICE;

typedef
NTSTATUS
NTAPI
DXGKDDI_STOP_DEVICE(_In_ const PVOID  MiniportDeviceContext);

typedef DXGKDDI_STOP_DEVICE                     *PDXGKDDI_STOP_DEVICE;

typedef
NTSTATUS
NTAPI
DXGKDDI_REMOVE_DEVICE(_In_ const PVOID  MiniportDeviceContext);

typedef DXGKDDI_REMOVE_DEVICE                   *PDXGKDDI_REMOVE_DEVICE;

typedef
NTSTATUS
NTAPI
DXGKDDI_DISPATCH_IO_REQUEST(_In_ const PVOID              MiniportDeviceContext,
                            _In_ ULONG                    VidPnSourceId,
                            _In_ PVIDEO_REQUEST_PACKET    VideoRequestPacket);

typedef DXGKDDI_DISPATCH_IO_REQUEST             *PDXGKDDI_DISPATCH_IO_REQUEST;

typedef
BOOLEAN
NTAPI
DXGKDDI_INTERRUPT_ROUTINE(_In_ const PVOID  MiniportDeviceContext,
                          _In_ ULONG        MessageNumber);

typedef DXGKDDI_INTERRUPT_ROUTINE               *PDXGKDDI_INTERRUPT_ROUTINE;

typedef
VOID
NTAPI
DXGKDDI_DPC_ROUTINE(_In_ const PVOID MiniportDeviceContext);

typedef DXGKDDI_DPC_ROUTINE                     *PDXGKDDI_DPC_ROUTINE;

typedef
NTSTATUS
NTAPI
DXGKDDI_QUERY_CHILD_RELATIONS(_In_ const PVOID                                                    MiniportDeviceContext,
                              _Inout_updates_bytes_(ChildRelationsSize) PDXGK_CHILD_DESCRIPTOR    ChildRelations,
                              _In_ ULONG                                                          ChildRelationsSize);

typedef DXGKDDI_QUERY_CHILD_RELATIONS           *PDXGKDDI_QUERY_CHILD_RELATIONS;

typedef
NTSTATUS
DXGKDDI_QUERY_CHILD_STATUS(
    _In_ const PVOID              MiniportDeviceContext,
    _Inout_ PDXGK_CHILD_STATUS    ChildStatus,
    _In_ BOOLEAN                  NonDestructiveOnly);

typedef DXGKDDI_QUERY_CHILD_STATUS              *PDXGKDDI_QUERY_CHILD_STATUS;

typedef
NTSTATUS
DXGKDDI_QUERY_DEVICE_DESCRIPTOR(
    _In_ const PVOID                  MiniportDeviceContext,
    _In_ ULONG                        ChildUid,
    _Inout_ PDXGK_DEVICE_DESCRIPTOR   DeviceDescriptor);

typedef DXGKDDI_QUERY_DEVICE_DESCRIPTOR         *PDXGKDDI_QUERY_DEVICE_DESCRIPTOR;

typedef
NTSTATUS
DXGKDDI_SET_POWER_STATE(
    _In_ const PVOID          MiniportDeviceContext,
    _In_ ULONG                DeviceUid,
    _In_ DEVICE_POWER_STATE   DevicePowerState,
    _In_ POWER_ACTION         ActionType);

typedef DXGKDDI_SET_POWER_STATE                 *PDXGKDDI_SET_POWER_STATE;

typedef
NTSTATUS
DXGKDDI_NOTIFY_ACPI_EVENT(
    _In_ const PVOID      MiniportDeviceContext,
    _In_ DXGK_EVENT_TYPE  EventType,
    _In_ ULONG            Event,
    _In_ PVOID            Argument,
    _Out_ PULONG          AcpiFlags);

typedef DXGKDDI_NOTIFY_ACPI_EVENT               *PDXGKDDI_NOTIFY_ACPI_EVENT;

typedef
VOID
DXGKDDI_RESET_DEVICE(
    _In_ const PVOID  MiniportDeviceContext);

typedef DXGKDDI_RESET_DEVICE                    *PDXGKDDI_RESET_DEVICE;

typedef
VOID
DXGKDDI_UNLOAD(
    VOID);

typedef DXGKDDI_UNLOAD                          *PDXGKDDI_UNLOAD;

typedef
NTSTATUS
DXGKDDI_QUERY_INTERFACE(
    _In_ const PVOID          MiniportDeviceContext,
    _In_ PQUERY_INTERFACE     QueryInterface);

typedef DXGKDDI_QUERY_INTERFACE                 *PDXGKDDI_QUERY_INTERFACE;

typedef
VOID
DXGKDDI_CONTROL_ETW_LOGGING(
    _In_ BOOLEAN  Enable,
    _In_ ULONG    Flags,
    _In_ UCHAR    Level);

typedef DXGKDDI_CONTROL_ETW_LOGGING             *PDXGKDDI_CONTROL_ETW_LOGGING;

typedef
NTSTATUS
DXGKDDI_LINK_DEVICE(
    _In_ const PDEVICE_OBJECT   PhysicalDeviceObject,
    _In_ const PVOID            MiniportDeviceContext,
    _Inout_ PLINKED_DEVICE      LinkedDevice);

typedef DXGKDDI_LINK_DEVICE                     *PDXGKDDI_LINK_DEVICE;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

/*
 * Passed from Miniport -> DxgKrnl
 * Call backs Implemented by display only drivers.
 */
typedef struct _KMDDOD_INITIALIZATION_DATA {
    ULONG                                   Version;
    PDXGKDDI_ADD_DEVICE                     DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE                   DxgkDdiStartDevice;
    PDXGKDDI_STOP_DEVICE                    DxgkDdiStopDevice;
    PDXGKDDI_REMOVE_DEVICE                  DxgkDdiRemoveDevice;
    PDXGKDDI_DISPATCH_IO_REQUEST            DxgkDdiDispatchIoRequest;
    PDXGKDDI_INTERRUPT_ROUTINE              DxgkDdiInterruptRoutine;
    PDXGKDDI_DPC_ROUTINE                    DxgkDdiDpcRoutine;
    PDXGKDDI_QUERY_CHILD_RELATIONS          DxgkDdiQueryChildRelations;
    PDXGKDDI_QUERY_CHILD_STATUS             DxgkDdiQueryChildStatus;
    PDXGKDDI_QUERY_DEVICE_DESCRIPTOR        DxgkDdiQueryDeviceDescriptor;
    PDXGKDDI_SET_POWER_STATE                DxgkDdiSetPowerState;
    PDXGKDDI_NOTIFY_ACPI_EVENT              DxgkDdiNotifyAcpiEvent;
    PDXGKDDI_RESET_DEVICE                   DxgkDdiResetDevice;
    PDXGKDDI_UNLOAD                         DxgkDdiUnload;
    PDXGKDDI_QUERY_INTERFACE                DxgkDdiQueryInterface;
    PDXGKDDI_CONTROL_ETW_LOGGING            DxgkDdiControlEtwLogging;
    PDXGKDDI_QUERYADAPTERINFO               DxgkDdiQueryAdapterInfo;
    PDXGKDDI_SETPALETTE                     DxgkDdiSetPalette;
    PDXGKDDI_SETPOINTERPOSITION             DxgkDdiSetPointerPosition;
    PDXGKDDI_SETPOINTERSHAPE                DxgkDdiSetPointerShape;
    PDXGKDDI_ESCAPE                         DxgkDdiEscape;
    //TODO:
} KMDDOD_INITIALIZATION_DATA, *PKMDDOD_INITIALIZATION_DATA;

#endif // DXGKDDI_INTERFACE_VERSION_WIN8

/*
 * Passed from Miniport -> DxgKrnl
 * Call backs Implemented by full WDDM drivers.
 */
typedef struct _DRIVER_INITIALIZATION_DATA {
    ULONG                                    Version;
    PDXGKDDI_ADD_DEVICE                      DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE                    DxgkDdiStartDevice;
    PDXGKDDI_STOP_DEVICE                     DxgkDdiStopDevice;
    PDXGKDDI_REMOVE_DEVICE                   DxgkDdiRemoveDevice;
    PDXGKDDI_DISPATCH_IO_REQUEST             DxgkDdiDispatchIoRequest;
    PDXGKDDI_INTERRUPT_ROUTINE               DxgkDdiInterruptRoutine;
    PDXGKDDI_DPC_ROUTINE                     DxgkDdiDpcRoutine;
    PDXGKDDI_QUERY_CHILD_RELATIONS           DxgkDdiQueryChildRelations;
    PDXGKDDI_QUERY_CHILD_STATUS              DxgkDdiQueryChildStatus;
    PDXGKDDI_QUERY_DEVICE_DESCRIPTOR         DxgkDdiQueryDeviceDescriptor;
    PDXGKDDI_SET_POWER_STATE                 DxgkDdiSetPowerState;
    PDXGKDDI_NOTIFY_ACPI_EVENT               DxgkDdiNotifyAcpiEvent;
    PDXGKDDI_RESET_DEVICE                    DxgkDdiResetDevice;
    PDXGKDDI_UNLOAD                          DxgkDdiUnload;
    PDXGKDDI_QUERY_INTERFACE                 DxgkDdiQueryInterface;
    PDXGKDDI_CONTROL_ETW_LOGGING             DxgkDdiControlEtwLogging;
    PDXGKDDI_QUERYADAPTERINFO                DxgkDdiQueryAdapterInfo;
    PDXGKDDI_CREATEDEVICE                    DxgkDdiCreateDevice;
    PDXGKDDI_CREATEALLOCATION                DxgkDdiCreateAllocation;
    PDXGKDDI_DESTROYALLOCATION               DxgkDdiDestroyAllocation;
    PDXGKDDI_DESCRIBEALLOCATION              DxgkDdiDescribeAllocation;
    PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA DxgkDdiGetStandardAllocationDriverData;
    PDXGKDDI_ACQUIRESWIZZLINGRANGE           DxgkDdiAcquireSwizzlingRange;
    PDXGKDDI_RELEASESWIZZLINGRANGE           DxgkDdiReleaseSwizzlingRange;
    PDXGKDDI_PATCH                           DxgkDdiPatch;
    PDXGKDDI_SUBMITCOMMAND                   DxgkDdiSubmitCommand;
    PDXGKDDI_PREEMPTCOMMAND                  DxgkDdiPreemptCommand;
    PDXGKDDI_BUILDPAGINGBUFFER               DxgkDdiBuildPagingBuffer;
    PDXGKDDI_SETPALETTE                      DxgkDdiSetPalette;
    PDXGKDDI_SETPOINTERPOSITION              DxgkDdiSetPointerPosition;
    PDXGKDDI_SETPOINTERSHAPE                 DxgkDdiSetPointerShape;
    PDXGKDDI_RESETFROMTIMEOUT                DxgkDdiResetFromTimeout;
    PDXGKDDI_RESTARTFROMTIMEOUT              DxgkDdiRestartFromTimeout;
    PDXGKDDI_ESCAPE                          DxgkDdiEscape;
    PDXGKDDI_COLLECTDBGINFO                  DxgkDdiCollectDbgInfo;
    PDXGKDDI_QUERYCURRENTFENCE               DxgkDdiQueryCurrentFence;
    PDXGKDDI_ISSUPPORTEDVIDPN                DxgkDdiIsSupportedVidPn;
    PDXGKDDI_RECOMMENDFUNCTIONALVIDPN        DxgkDdiRecommendFunctionalVidPn;
    PDXGKDDI_ENUMVIDPNCOFUNCMODALITY         DxgkDdiEnumVidPnCofuncModality;
    PDXGKDDI_SETVIDPNSOURCEADDRESS           DxgkDdiSetVidPnSourceAddress;
    PDXGKDDI_SETVIDPNSOURCEVISIBILITY        DxgkDdiSetVidPnSourceVisibility;
    PDXGKDDI_COMMITVIDPN                     DxgkDdiCommitVidPn;
    PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH    DxgkDdiUpdateActiveVidPnPresentPath;
    PDXGKDDI_RECOMMENDMONITORMODES           DxgkDdiRecommendMonitorModes;
    PDXGKDDI_RECOMMENDVIDPNTOPOLOGY          DxgkDdiRecommendVidPnTopology;
    PDXGKDDI_GETSCANLINE                     DxgkDdiGetScanLine;
    PDXGKDDI_STOPCAPTURE                     DxgkDdiStopCapture;
    PDXGKDDI_CONTROLINTERRUPT                DxgkDdiControlInterrupt;
    PDXGKDDI_CREATEOVERLAY                   DxgkDdiCreateOverlay;
    PDXGKDDI_DESTROYDEVICE                   DxgkDdiDestroyDevice;
    PDXGKDDI_OPENALLOCATIONINFO              DxgkDdiOpenAllocation;
    PDXGKDDI_CLOSEALLOCATION                 DxgkDdiCloseAllocation;
    PDXGKDDI_RENDER                          DxgkDdiRender;
    PDXGKDDI_PRESENT                         DxgkDdiPresent;
    PDXGKDDI_UPDATEOVERLAY                   DxgkDdiUpdateOverlay;
    PDXGKDDI_FLIPOVERLAY                     DxgkDdiFlipOverlay;
    PDXGKDDI_DESTROYOVERLAY                  DxgkDdiDestroyOverlay;
    PDXGKDDI_CREATECONTEXT                   DxgkDdiCreateContext;
    PDXGKDDI_DESTROYCONTEXT                  DxgkDdiDestroyContext;
    PDXGKDDI_LINK_DEVICE                     DxgkDdiLinkDevice;
    PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT   DxgkDdiSetDisplayPrivateDriverFormat;
// > DXGKDDI_INTERFACE_VERSION_VISTA
} DRIVER_INITIALIZATION_DATA, *PDRIVER_INITIALIZATION_DATA;

#endif // _DISPMPRT_H_

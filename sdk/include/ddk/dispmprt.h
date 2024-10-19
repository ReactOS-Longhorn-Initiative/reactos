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

/*
 * Passed from DxgKrnl -> Miniport
 * Implemented by DxgKrnl itself.
 */
typedef struct _DXGKRNL_INTERFACE {
    ULONG                                   Size;
    ULONG                                   Version;
    HANDLE                                  DeviceHandle;
    //TODO:
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

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

/*
 * Passed from Miniport -> DxgKrnl
 * Call backs Implemented by display only drivers.
 */
typedef struct _KMDDOD_INITIALIZATION_DATA {
    ULONG                                   Version;
    PDXGKDDI_ADD_DEVICE                     DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE                   DxgkDdiStartDevice;
    //TODO:
} KMDDOD_INITIALIZATION_DATA, *PKMDDOD_INITIALIZATION_DATA;

#endif // DXGKDDI_INTERFACE_VERSION_WIN8

/*
 * Passed from Miniport -> DxgKrnl
 * Call backs Implemented by full WDDM drivers.
 */
typedef struct _DRIVER_INITIALIZATION_DATA {
    ULONG                                   Version;
    PDXGKDDI_ADD_DEVICE                     DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE                   DxgkDdiStartDevice;
    //TODO:
} DRIVER_INITIALIZATION_DATA, *PDRIVER_INITIALIZATION_DATA;

#endif // _DISPMPRT_H_

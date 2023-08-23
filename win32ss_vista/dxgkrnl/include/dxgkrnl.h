/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Core Header
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <ntddk.h>
#include <windef.h>
#include <ntstatus.h>

#ifdef __cplusplus
extern "C"
{
	#endif
NTKERNELAPI
NTSTATUS
NTAPI
ObCloseHandle(
    _In_ HANDLE Handle,
    _In_ KPROCESSOR_MODE AccessMode
);
#ifdef __cplusplus
}
#endif
#include <stdio.h>
#include "../../include/locd3d.h"
#include <ntddvdeo.h>
/* Create an IO request to fill out the function pointer list */
#define IOCTL_VIDEO_DDI_FUNC_REGISTER \
	CTL_CODE( FILE_DEVICE_VIDEO, 0xF, METHOD_NEITHER, FILE_ANY_ACCESS  )

//FIXME: ReactOS Specific
#define IOCTL_VIDEO_I_AM_REACTOS \
	CTL_CODE( FILE_DEVICE_VIDEO, 0xB, METHOD_NEITHER, FILE_ANY_ACCESS  )


/* FIXME: This needs some real investigation - this is this stupid  */
typedef struct _DXGKRNL_PRIVATE_EXTENSION
{
	DRIVER_INITIALIZATION_DATA DriverInitData;
	PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT MiniportFdo;
    PDEVICE_OBJECT MiniportPdo;
	INTERFACE_TYPE AdapterInterfaceType;
    PVOID MiniportContext;
	UNICODE_STRING RegistryPath;
    UNICODE_STRING NewRegistryPath;
	PDEVICE_OBJECT NextDeviceObject;
	ULONG SystemIoBusNumber;
	ULONG SystemIoSlotNumber;
} DXGKRNL_PRIVATE_EXTENSION, *PDXGKRNL_PRIVATE_EXTENSION;


#include "rdport.h"

#if __cplusplus
#include "classes/dxgdevice.hxx"
/* HAS TO BE AFTER VIDPN AND FRIENDS*/
#include "classes/dxgadapter.hxx"
/* HAS TO BE THE BOTTOM*/
#include "classes/dxgglobal.hxx"
#endif
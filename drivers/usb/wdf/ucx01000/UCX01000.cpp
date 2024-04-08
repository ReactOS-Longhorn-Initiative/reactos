/*
 * PROJECT:         ReactOS WDF Modern USB Stack
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         UCX01000 Startup
 * COPYRIGHT:       Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */
#define USB3MOMENT 1
#include "UCX01000.h"
extern "C"
{
 #include "../include/ucxclass.h"
}

#include <debug.h>

NTSTATUS
NTAPI
UcxClassBindClient(_In_ PWDF_CLASS_BIND_INFO WdfClassBindInfo,
                   _In_ void** WdfComponentGlobals)
{
    return 0;
}

VOID
NTAPI
UcxClassUnbindClient(_In_ PWDF_CLASS_BIND_INFO WdfClassBindInfo,
                     _In_ void** WdfComponentGlobals)
{
    ExFreePool(WdfClassBindInfo->ClassBindInfo);
}

NTSTATUS
NTAPI
UcxClassInitialize(VOID)
{
    /* Not sure what this is doing in windows... */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UcxClassUninitialize(VOID)
{
    /* Not sure what this is doing in windows... */
    return STATUS_SUCCESS;
}

#if 0
typedef struct _WDF_CLASS_LIBRARY_INFO {
    ULONG Size;
    WDF_CLASS_VERSION Version;
    PFN_CLASS_LIBRARY_INIT ClassLibraryInitialize;
    PFN_CLASS_LIBRARY_DEINIT ClassLibraryDeinitialize;
    PFN_CLASS_LIBRARY_BIND_CLIENT ClassLibraryBindClient;
    PFN_CLASS_LIBRARY_UNBIND_CLIENT ClassLibraryUnbindClient;
} WDF_CLASS_LIBRARY_INFO, *PWDF_CLASS_LIBRARY_INFO;
#endif

typedef struct _UCX01000_DRIVER_EXTENSION {
    ULONG               Version;
    PDRIVER_OBJECT      DriverObject;
    ULONG               Flags;
} UCX01000_DRIVER_EXTENSION, *PUCX01000_DRIVER_EXTENSION;

#define USBHUB_TAG '0XCU'

DECLARE_CONST_UNICODE_STRING(SDDL_DEVOBJ_KERNEL_ONLY, L"D:P");


WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UCX01000_DRIVER_EXTENSION, DriverGetExtension)
PVOID PublicWdfDriverUcxContext;
EXTERN_C
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    __debugbreak();
    NTSTATUS                Status;
    WDF_DRIVER_CONFIG       config;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDRIVER               driverObject = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UCX01000_DRIVER_EXTENSION);
    WDF_DRIVER_CONFIG_INIT(&config, NULL);

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             &driverObject);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("Failed to create WdfDevice\n");
        return Status;
    }

    PublicWdfDriverUcxContext = WdfObjectGetTypedContextWorker(driverObject,
                                      attributes.ContextTypeInfo);

 
    PWDFDEVICE_INIT   pWdfDeviceInit ;
    pWdfDeviceInit = WdfControlDeviceInitAllocate(driverObject, &SDDL_DEVOBJ_KERNEL_ONLY);
    if (!pWdfDeviceInit)
    {
        DPRINT1("pWdfDeviceInit: is null \n");
        __debugbreak();
        return 1;
    }

    return STATUS_SUCCESS;
}

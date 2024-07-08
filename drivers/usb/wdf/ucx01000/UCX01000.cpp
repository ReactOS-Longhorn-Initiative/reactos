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

#define USBHUB_TAG '0XCU'


#include <debug.h>

/* this is the crappy part.... the exports. early win10 seems to have 22 funcs*/
///UcxExports:
/* 
  UcxIoDeviceControl
  UcxControllerCreate 
  UcxControllerNeedsReset 
  UcxControllerResetComplete 
  UcxControllerSetFailed 
  UcxRootHubCreate 
  UcxRootHubPortChanged 
  UcxUsbDeviceCreate 
  UcxUsbDeviceInitSetEventCallbacks 
  UcxUsbDeviceRemoteWakeNotification 
  UcxEndpointCreate 
  UcxEndpointGetStaticStreamsReferenced 
  UcxEndpointNeedToCancelTransfers 
  UcxEndpointInitSetEventCallbacks 
  UcxDefaultEndpointInitSetEventCallbacks 
  UcxEndpointSetWdfIoQueue 
  UcxEndpointPurgeComplete 
  UcxEndpointAbortComplete 
  UcxEndpointNoPingResponseError 
  UcxStaticStreamsSetStreamInfo 
  UcxStaticStreamsCreate 
  UcxInitializeDeviceInit 
*/

NTSTATUS
NTAPI
UcxClassBindClient(_In_ PWDF_CLASS_BIND_INFO WdfClassBindInfo,
                   _In_ void** WdfComponentGlobals)
{
    PVOID ClassBindInfoLoc;
    __debugbreak();

    if (WdfClassBindInfo->FunctionTableCount != 22)
    {
        // im feeling 22
        __debugbreak();
    }

    //Copy the exports ... will end up making this a better setup
    RtlCopyMemory(WdfClassBindInfo->FunctionTable, UcxExports, 0x58);

    // Putting some garbage here since todo
    ClassBindInfoLoc = ExAllocatePoolWithTag(NonPagedPool, 0x8, USBHUB_TAG);
     if ( ClassBindInfoLoc )
     {
         WdfClassBindInfo->ClassBindInfo = ClassBindInfoLoc;
     }
    __debugbreak();
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
      __debugbreak();
    /* Not sure what this is doing in windows... */
    return STATUS_SUCCESS;
}

VOID
NTAPI
UcxClassUninitialize(VOID)
{
    __debugbreak();
    /* Not sure what this is doing in windows... */
}
 

typedef struct _UCX01000_DRIVER_EXTENSION {
    ULONG               Version;
    PDRIVER_OBJECT      DriverObject;
    ULONG               Flags;
} UCX01000_DRIVER_EXTENSION, *PUCX01000_DRIVER_EXTENSION;


DECLARE_CONST_UNICODE_STRING(SDDL_DEVOBJ_KERNEL_ONLY, L"D:P");
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UCX01000_DRIVER_EXTENSION, DriverGetExtension)



 /* We're going to mimic this but I can only think these are random generated at runtime
  * My theory is that the MS build system generates these sigs, So maybe they're unique between builds...
  * Long term we won't copy this behavior because fuck this behavior
  */
typedef enum _UCX_SIG
{
  SigAddress0OwnerShipQueueContext = 0x434F3041,
  SigRHPdoContext = 0x6F645052,
  SigRootHubControlQueueContext = 0x75514352,
  SigRootHubInterruptQueueContext = 0x75514952,
  SigStreamContext = 0x6D727453,
  SigUcxControllerContext = 0x6C727443,
  SigUcxEndpointContext = 0x70646E45,
  SigUcxEndpointInit = 0x74697045,
  SigUcxFunctionInfo = 0x636E7546,
  SigUcxForwardProgressWorkItem = 0x69577046,
  SigUcxRootHubContext = 0x63627548,
  SigUcxSStreamsInit = 0x696D7353,
  SigUcxStreamsContext = 0x6D747353,
  SigUcxUsbDeviceContext = 0x69766544,
  SigUcxUsbDeviceInit = 0x74696544,
  SigUsbdiHandleData = 0x68644455,
  SigUsbPipeInfo = 0x65706950,
  SigXrb = 0x2E425258,
  SigWdfDeviceUcxContext = 0x78554457,
  SigWdfDriverUcxContext = 0x78557244,
} UCX_SIG, *PUCX_SIG;

typedef struct _WDFDRIVER_UCX_CONTEXT
{
  UCX_SIG Sig;
  WDFDEVICE UcxWdfDevice;
  LIST_ENTRY UcxControllerListHead;
  KSPIN_LOCK UcxControllerListLock;
  UINT32 UcxControllerListCount;
  BOOLEAN SleepStudyEnabled; // We're not doing this.
} WDFDRIVER_UCX_CONTEXT, *PWDFDRIVER_UCX_CONTEXT;

WDF_CLASS_LIBRARY_INFO GlobalClassInfo;
PWDFDRIVER_UCX_CONTEXT PublicWdfDriverUcxContext;
#define UCX_DEVICE_NAME L"\\Device\\UCX"

EXTERN_C
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS                Status;
    WDF_DRIVER_CONFIG       config;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDRIVER               driverObject = NULL;
    UNICODE_STRING      ucxDeviceName;

    //Just a temp buffer
    wchar_t ucxDeviceNamebuffer[100];
    ucxDeviceName.Length = 13107200;
    ucxDeviceName.Buffer = ucxDeviceNamebuffer;



    //class
    GlobalClassInfo.Size = 0x20;
    GlobalClassInfo.Version = {1,1,0};
    GlobalClassInfo.ClassLibraryBindClient = UcxClassBindClient;
    GlobalClassInfo.ClassLibraryInitialize = UcxClassInitialize;
    GlobalClassInfo.ClassLibraryUnbindClient = UcxClassUnbindClient;
    GlobalClassInfo.ClassLibraryDeinitialize = UcxClassUninitialize;
    //endofclass

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

    PublicWdfDriverUcxContext = (PWDFDRIVER_UCX_CONTEXT)WdfObjectGetTypedContextWorker(driverObject,
                                                            attributes.ContextTypeInfo);

    PublicWdfDriverUcxContext->Sig = SigWdfDriverUcxContext;
    InitializeListHead(&PublicWdfDriverUcxContext->UcxControllerListHead);
    KeInitializeSpinLock(&PublicWdfDriverUcxContext->UcxControllerListLock);
    PublicWdfDriverUcxContext->UcxControllerListCount = 0;
    PWDFDEVICE_INIT   pWdfDeviceInit ;
    pWdfDeviceInit = WdfControlDeviceInitAllocate(driverObject, &SDDL_DEVOBJ_KERNEL_ONLY);
    if (!pWdfDeviceInit)
    {
        DPRINT1("pWdfDeviceInit: is null \n");
        __debugbreak();
        return 1;
    }
    ULONG Index = 0;
    while (1)
    {
        Status = RtlUnicodeStringPrintf(&ucxDeviceName, L"%s%d", UCX_DEVICE_NAME, Index);
        Index++;
        if (!NT_SUCCESS(Status))
            break;
        Status = WdfDeviceInitAssignName(pWdfDeviceInit, &ucxDeviceName);
        if (!NT_SUCCESS(Status))
            __debugbreak();
        Status = WdfDeviceCreate(&pWdfDeviceInit, NULL, &PublicWdfDriverUcxContext->UcxWdfDevice);
        if ( Status != STATUS_OBJECT_NAME_COLLISION )
            goto CONTINUE_EXEC;
        if ( Index >= 0x64 )
            __debugbreak();
    }
CONTINUE_EXEC:
    if (!NT_SUCCESS(Status))
    {
        __debugbreak();
    }
    WdfControlFinishInitializing(PublicWdfDriverUcxContext->UcxWdfDevice);
    Status = WdfRegisterClassLibrary(&GlobalClassInfo, RegistryPath, &ucxDeviceName);
    if (!NT_SUCCESS(Status))
    {
        __debugbreak();
    }

    __debugbreak();
    return STATUS_SUCCESS;
}

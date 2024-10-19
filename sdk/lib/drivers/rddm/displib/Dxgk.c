/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     DISPLIB static library
 * COPYRIGHT:   Copyright 2023 Justin Miller <justinmiller100@gmail.com>
 */

#include <ntddk.h>
#include <windef.h>
#include <winerror.h>
#include <stdio.h>
#include <dispmprt.h>
#include <wdm.h>
#include <reactos/rddm/rddm_private.h>
#define NDEBUG
#include <debug.h>

VOID
RDDM_UnloadDxgkrnl(_In_ PUNICODE_STRING DxgkrnlServiceName)
{
    RtlInitUnicodeString(DxgkrnlServiceName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\DXGKrnl");
    ZwUnloadDriver(DxgkrnlServiceName);
}

typedef enum _RDDM_INTERFACE_CHOICE
{
  FullWddm,
  KmDod
} RDDM_INTERFACE_CHOICE;

RDDM_INTERFACE_CHOICE RddmChoice = 0;

/*
 * At least Two IOCTRL varients exist, for now just implement the Full WDDM.
 */
ULONG
RDDM_FindIoControlCode()
{
    if (RddmChoice == FullWddm)
    {
        return IOCTL_VIDEO_DDI_FUNC_REGISTER;
    }
    else
    {
        UNIMPLEMENTED;
        return 0;
    }
}

/*
 * Some information:
 * First off, unless this library is totally implemented 100% stable, WDDM drivers
 * compiled with the ReactOS toolchain will never start. This is because
 * as far as i can tell the managment of DXGKRNL is mostly done within this library
 * Starting the driver, prepping function call backs to pass to dxgkrnl are all done here.
 *
 * this also means that DXGKRNL as a service doesn't start unless a driver has invoked it
 * as of this recent commit this behavior is now true on ReactOS as well :).
 */

NTSTATUS
NTAPI
DxgkInitialize(
  _In_ PDRIVER_OBJECT              DriverObject,
  _In_ PUNICODE_STRING             RegistryPath,
  _In_ PDRIVER_INITIALIZATION_DATA DriverInitializationData)
{
    /* This is internal and gets filled out VIA a IOCTRL */
    NTSTATUS (NTAPI *DpiInitialize)(PDRIVER_OBJECT, PUNICODE_STRING, PDRIVER_INITIALIZATION_DATA);
    UNICODE_STRING DxgkrnlServiceName;
    DEVICE_OBJECT *DxgkrnlDeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    KEVENT Event;
    IRP *Irp;

    DxgkrnlDeviceObject = 0;
    DpiInitialize = NULL;
    DPRINT("Displib: DxgkInitialize - Starting a WDDM Driver\n");
    if (!DriverObject,
        !RegistryPath)
    {
        DPRINT("DriverObject or RegistryPath is NULL\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (DriverInitializationData)
    {

        /* It appears Windows will actually fail if the Miniport is below a specific version - we don't care */
        DPRINT("Displib: This WDDM Miniport version is %X", DriverInitializationData->Version);

        /* First load DXGKrnl itself */
        RtlInitUnicodeString(&DxgkrnlServiceName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\DXGKrnl");
        Status = ZwLoadDriver(&DxgkrnlServiceName);
        if (Status == STATUS_SUCCESS)
        {
            /* Okay we suceeded, Go ahead and grab the DxgkrnlDeviceObject */
            RtlInitUnicodeString(&DeviceName, L"\\Device\\DxgKrnl");
            Status = IoGetDeviceObjectPointer(&DeviceName,
                                              FILE_ALL_ACCESS,
                                              (PFILE_OBJECT*)&DriverInitializationData,
                                              &DxgkrnlDeviceObject);
            if (Status != STATUS_SUCCESS)
            {
                RDDM_UnloadDxgkrnl(&DxgkrnlServiceName);
                return Status;
            }
            /* Grab a function pointer to DpiInitialize via IOCTRL */
            KeInitializeEvent(&Event, NotificationEvent, 0);
            Irp = IoBuildDeviceIoControlRequest(RDDM_FindIoControlCode(),
                                                DxgkrnlDeviceObject,
                                                NULL,
                                                0,
                                                &DpiInitialize,
                                                sizeof(DpiInitialize),
                                                TRUE,
                                                &Event,
                                                &IoStatusBlock);

            /* Can't continue without being able to call this routine */
            if (!Irp)
            {
              Status = STATUS_INSUFFICIENT_RESOURCES;
              /* So fail */
              RDDM_UnloadDxgkrnl(&DxgkrnlServiceName);
              return Status;
            }

            Status = IofCallDriver(DxgkrnlDeviceObject, Irp);
        }

        /* Execute the thing */
        if ( Status != STATUS_SUCCESS)
        {
            RDDM_UnloadDxgkrnl(&DxgkrnlServiceName);
        }
        else
        {
          DPRINT("Displib: Custom RDDM Driver has passed - IOCTL_VIDEO_DDI_FUNC_REGISTER sent\n");
          Status = DpiInitialize(DriverObject, RegistryPath, DriverInitializationData);
          DPRINT1("Displib: return from DpiInitialize Success\n");
          return Status;
        }
        return Status;
    }
    return STATUS_INVALID_PARAMETER;
}

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)

NTSTATUS
NTAPI
DxgkInitializeDisplayOnlyDriver(
  _In_ PDRIVER_OBJECT              DriverObject,
  _In_ PUNICODE_STRING             RegistryPath,
  _In_ PKMDDOD_INITIALIZATION_DATA KmdDodInitializationData)
{
    RddmChoice = KmDod;
    /* We can just Call this */
    return DxgkInitialize(DriverObject, RegistryPath, (PDRIVER_INITIALIZATION_DATA)KmdDodInitializationData);
}

#endif

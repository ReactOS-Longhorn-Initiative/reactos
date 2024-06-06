/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ReactX Kernel Driver Entry Point
 * COPYRIGHT:   Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#include <rxgkrnl.h>
//#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
RxgkInternalDeviceControl(
    _In_ DEVICE_OBJECT *DeviceObject,
    _In_ IRP *Irp)
{
    ULONG IoControlCode;
    PVOID *OutputBuffer;
    PIO_STACK_LOCATION IrpStack;

    PAGED_CODE();

    /* First let's grab the IOCTRL code */
    IrpStack = Irp->Tail.Overlay.CurrentStackLocation;
    IoControlCode = IrpStack->Parameters.Read.ByteOffset.LowPart;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    switch (IoControlCode)
    {
        case IOCTL_VIDEO_DDI_FUNC_REGISTER:
            /*
             * Grab a reference to the InitializeMiniport function so we can acquire the Miniport
             * callback list and continue setup
             */
            OutputBuffer = (PVOID*)Irp->UserBuffer;
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            *OutputBuffer = (PVOID)NULL;
            UNIMPLEMENTED;
            __debugbreak();
            break;
        default:
            DPRINT("RxgkInternalDeviceControl: unknown IOCTRL Code: %X\n", IoControlCode);
            break;
    }

    IofCompleteRequest(Irp, 0);
    return STATUS_SUCCESS;
}

VOID
NTAPI
RxgkUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();
    UNIMPLEMENTED;
    __debugbreak();
}

NTSTATUS
NTAPI
RxgkUnused(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PAGED_CODE();
    DPRINT("DxgkCreateClose: called\n");
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IofCompleteRequest(Irp, 0);
    return STATUS_SUCCESS;
}

EXTERN_C
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    UNICODE_STRING DestinationString;
    PDEVICE_OBJECT DxgkrnlDeviceObject;

    /* First fillout dispatch table */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = RxgkUnused;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = RxgkUnused;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = RxgkInternalDeviceControl;
    DriverObject->DriverUnload = RxgkUnload;

    RtlInitUnicodeString(&DestinationString, L"\\Device\\DxgKrnl");
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DestinationString,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DxgkrnlDeviceObject);

    if (!NT_SUCCESS(Status))
        DPRINT1("DriverEntry Failed with status %X", Status);

    DPRINT1("ReactOS Display Driver Model:\n");
    DPRINT1("Targetting Version: 0x%X\n", 0x1000);

    return Status;
}

/*
 * PROJECT:     ReactOS Display Driver Model
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     DxgKrnl basic driver handling
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include <include/dxgkrnl.h>
//#define NDEBUG
#include <debug.h>

static PDRIVER_OBJECT DxgkDriverObject;
static PDEVICE_OBJECT DxgkDeviceObject;
static UNICODE_STRING DxgkRegistryPath;

/*
 * @ HALF-IMPLEMENTED
 * Internal Device Control Handler
 * IOCTL_VIDEO_I_AM_REACTOS: -> ReactOS Specific, used to trigger StartAdapterThread,
 * However if this IOCTRL is not valid, we know we're using a Windows DxgKrnl.
 * IOCTL_VIDEO_DDI_FUNC_REGISTER -> Matches Windows value, Used to create register miniport callbacks.
 * IOCTL_VIDEO_KMDOD_DDI_REGISTER -> Matches Windows value, Used to create register KMDOD callbacks.
 * IOCTL_VIDEO_CDD_FUNC_REGISTER -> Matches Windows value, Used to send CDD callbacks to cdd.dll
 */
NTSTATUS
NTAPI
DxgkHandleInternalDeviceControl(_In_ DEVICE_OBJECT *DeviceObject, _Inout_ IRP *Irp)
{
    ULONG IoControlCode;
    PVOID *OutputBuffer;
    PIO_STACK_LOCATION IrpStack;

    PAGED_CODE();

    /* First let's grab the IOCTRL code */
    IrpStack = Irp->Tail.Overlay.CurrentStackLocation;
    IoControlCode = IrpStack->Parameters.Read.ByteOffset.LowPart;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

    /* Handle all internal IOCTRLs */
    switch (IoControlCode)
    {
        case IOCTL_VIDEO_CDD_FUNC_REGISTER:
            DPRINT("DxgkInternalDeviceControl: IOCTL_VIDEO_CDD_FUNC_REGISTER\n");
            UNIMPLEMENTED;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        case IOCTL_VIDEO_KMDOD_DDI_REGISTER:
            DPRINT("DxgkInternalDeviceControl: IOCTL_VIDEO_KMDOD_DDI_REGISTER\n");
            OutputBuffer = Irp->UserBuffer;
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            *OutputBuffer = (PVOID)DxgkPortInitializeDodMiniport;
            break;
        case IOCTL_VIDEO_DDI_FUNC_REGISTER:
            DPRINT("DxgkInternalDeviceControl: IOCTL_VIDEO_DDI_FUNC_REGISTER\n");
            OutputBuffer = Irp->UserBuffer;
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            *OutputBuffer = (PVOID)DxgkPortInitializeMiniport;
            break;
        case IOCTL_VIDEO_I_AM_REACTOS:
            DPRINT("DxgkInternalDeviceControl: IOCTL_VIDEO_I_AM_REACTOS\n");
            /* Call Start Adapter */
            //TODO: StartAdapterThread();
            UNIMPLEMENTED;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        default:
            DPRINT("DxgkInternalDeviceControl: unknown IOCTRL Code: %X\n", IoControlCode);
            break;
    }

    /* Check Irp Status and return if it's not STATUS_SUCCESS */
    if (Irp->IoStatus.Status != STATUS_SUCCESS)
    {
        DPRINT("DxgkInternalDeviceControl: Irp Status: %X\n", Irp->IoStatus.Status);
    }

    IofCompleteRequest(Irp, 0);
    return Irp->IoStatus.Status;
}

/*
 * @ HALF-IMPLEMENTED
 * TODO: Let's check for extra things to clean up
 */
NTSTATUS
NTAPI
DxgkUnload(_In_ DRIVER_OBJECT *DriverObject)
{
    PAGED_CODE();
    DPRINT("DxgkUnload: Entry Point\n");

    /* Unload DriverObject */
    IoDeleteDevice(DriverObject->DeviceObject);
    return STATUS_SUCCESS;
}

/*
 * @ IMPLEMENTED
 * This doesn't do anything meaningful, we use internal IOCTRLs for
 * adding our devices and removing them.
 */
NTSTATUS
NTAPI
DxgkUnused(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    PAGED_CODE();
    DPRINT("DxgkCreateClose: Entry Point\n");
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IofCompleteRequest(Irp, 0);
    return STATUS_SUCCESS;
}

/*
 * C Entrypoint for Dxgkrnl driver
 * @ IMPLEMENTD
 */
NTSTATUS
NTAPI
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    UNICODE_STRING DxgKrnlUnicode;

    PAGED_CODE();
    if (!DriverObject || !RegistryPath)
        return STATUS_INVALID_PARAMETER;

    DxgkDriverObject = DriverObject;
    DPRINT1("---------------------------ReactOS Display Driver Model---------------------------\n");
    DPRINT1("Targetting Version: 0x%X\n", DXGKDDI_INTERFACE_VERSION_VISTA_SP1);

    /* Fill out DriverObject dispatches */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DxgkUnused;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DxgkUnused;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DxgkHandleInternalDeviceControl;
    DriverObject->DriverUnload = DxgkUnload;

    /* Grab DxgKrnl unicode */
    RtlCopyUnicodeString(&DxgkRegistryPath, RegistryPath);
    RtlInitUnicodeString(&DxgKrnlUnicode, L"\\Device\\DxgKrnl");

    /* Create Device */
    Status = IoCreateDevice(DriverObject,
                            0, // Let's allocate DeviceExtension later
                            &DxgKrnlUnicode,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DxgkDeviceObject);
    /* Check Status */
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to create device: 0x%X\n", Status);
        //FIXME: Probably should clean up those globals.
        return Status;
    }

    //TODO: Setup global instances here.
    return Status;
}
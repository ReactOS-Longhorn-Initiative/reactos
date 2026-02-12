#include "precomp.h"
#include "acpi_new.h"

NTSTATUS
NTAPI
AcpiNewAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS status;
    PDEVICE_OBJECT fdo;
    PACPI_NEW_FDO_EXTENSION fdoExt;

    status = IoCreateDevice(DriverObject,
                            sizeof(ACPI_NEW_FDO_EXTENSION),
                            NULL,
                            FILE_DEVICE_ACPI,
                            0,
                            FALSE,
                            &fdo);
    if (!NT_SUCCESS(status))
        return status;

    fdoExt = (PACPI_NEW_FDO_EXTENSION)fdo->DeviceExtension;
    RtlZeroMemory(fdoExt, sizeof(*fdoExt));
    fdoExt->Common.Type = AcpiNewDeviceFdo;
    fdoExt->Common.Self = fdo;
    fdoExt->PhysicalDeviceObject = PhysicalDeviceObject;
    ExInitializeFastMutex(&fdoExt->Mutex);
    InitializeListHead(&fdoExt->PdoList);
    InterlockedExchange(&fdoExt->EnumerationDirty, 1);

    fdoExt->LowerDevice = IoAttachDeviceToDeviceStack(fdo, PhysicalDeviceObject);
    if (!fdoExt->LowerDevice)
    {
        IoDeleteDevice(fdo);
        return STATUS_NO_SUCH_DEVICE;
    }

    fdo->Flags |= DO_POWER_PAGABLE;
    fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

VOID
NTAPI
AcpiNewUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
}

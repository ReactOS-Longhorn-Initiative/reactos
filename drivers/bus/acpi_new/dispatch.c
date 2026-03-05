#include "precomp.h"
#include "acpi_new.h"

NTSTATUS
NTAPI
AcpiNewDispatchPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    if (AcpiNewIsFdo(DeviceObject))
    {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(((PACPI_NEW_FDO_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
    }

    PoStartNextPowerIrp(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
AcpiNewDispatchCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
AcpiNewDispatchDefault(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    if (AcpiNewIsFdo(DeviceObject))
    {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(((PACPI_NEW_FDO_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice, Irp);
    }

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_SUPPORTED;
}

#include "precomp.h"
#include "acpi_new.h"

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = AcpiNewUnload;
    DriverObject->DriverExtension->AddDevice = AcpiNewAddDevice;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = AcpiNewDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = AcpiNewDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_PNP] = AcpiNewDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = AcpiNewDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AcpiNewDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = AcpiNewDispatchDefault;

    return AcpiNewCreateControlDevice(DriverObject);
}

#pragma once

NTSTATUS
NTAPI
RxgkPortInitializeMiniport(_In_ PDRIVER_OBJECT DriverObject,
                           _In_ PUNICODE_STRING SourceString,
                           _In_ PDRIVER_INITIALIZATION_DATA DriverInitData);
NTSTATUS
NTAPI
RxgkPortAddDevice(_In_    DRIVER_OBJECT *DriverObject,
                  _Inout_ DEVICE_OBJECT *PhysicalDeviceObject);

NTSTATUS
NTAPI
RxgkPortDriverUnload(_In_ PDRIVER_OBJECT DriverObject);

NTSTATUS
NTAPI
RxgkPortDispatchCreateDevice(_In_    PDEVICE_OBJECT DeviceObject,
                             _Inout_ PIRP Irp);

NTSTATUS
NTAPI
RxgkPortDispatchPnp(_In_ PDEVICE_OBJECT DeviceObject,
                    _In_ PVOID Tag);

PSTR
NTAPI
RxgkPortDispatchPower(_In_ PDEVICE_OBJECT DeviceObject,
                      _In_ PSTR MutableMessage);

NTSTATUS
NTAPI
RxgkPortDispatchIoctl(_In_    PDEVICE_OBJECT DeviceObject,
                      _Inout_ IRP *Irp);

NTSTATUS
NTAPI
RxgkPortDispatchInternalIoctl(_In_ PDEVICE_OBJECT DeviceObject,
                              _Inout_ IRP *Irp);
NTSTATUS
NTAPI
RxgkPortDispatchSystemControl(_In_ PDEVICE_OBJECT DeviceObject,
                              _In_ PVOID Tag);

NTSTATUS
NTAPI
RxgkPortDispatchCloseDevice(_In_ PDEVICE_OBJECT DeviceObject);

NTSTATUS
NTAPI
IntVideoPortAddDeviceMapLink(
    PRXGK_PRIVATE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
DxgkpCreateIds();

NTSTATUS
NTAPI
IntCreateRegistryPath(
    IN PCUNICODE_STRING DriverRegistryPath,
    IN ULONG DeviceNumber,
    OUT PUNICODE_STRING DeviceRegistryPath);

NTSTATUS
NTAPI
IntCreateNewRegistryPath(
    PRXGK_PRIVATE_EXTENSION DeviceExtension);


NTSTATUS
NTAPI
RxgkStartAdapter();

NTSTATUS
NTAPI
RxgkpSetupDxgkrnl(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath);

CODE_SEG("PAGE")
NTSTATUS
RxgkpQueryInterface(
    _In_ PRXGK_PRIVATE_EXTENSION RxgkpExtension,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Size);

#include "precomp.h"
#include "acpi_new.h"

#include <uacpi/event.h>

typedef struct _ACPI_NEW_NOTIFY_ENTRY
{
    LIST_ENTRY Link;
    BOOLEAN V2;
    union
    {
        PDEVICE_NOTIFY_CALLBACK V1;
        PDEVICE_NOTIFY_CALLBACK2 V2Cb;
    } Callback;
    PVOID NotificationContext;
} ACPI_NEW_NOTIFY_ENTRY, *PACPI_NEW_NOTIFY_ENTRY;

static
VOID
NTAPI
AcpiNewInterfaceReference(_In_ PVOID Context)
{
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    if (DeviceObject)
        ObReferenceObject(DeviceObject);
}

static
VOID
NTAPI
AcpiNewInterfaceDereference(_In_ PVOID Context)
{
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    if (DeviceObject)
        ObDereferenceObject(DeviceObject);
}

typedef struct _ACPI_NEW_GPE_CONTEXT
{
    ULONG GpeNumber;
    uacpi_gpe_triggering Trigger;
    PGPE_SERVICE_ROUTINE ServiceRoutine;
    PVOID ServiceContext;
} ACPI_NEW_GPE_CONTEXT, *PACPI_NEW_GPE_CONTEXT;

static
NTSTATUS
AcpiNewUacpiStatusToNtStatus(_In_ uacpi_status st)
{
    switch (st)
    {
    case UACPI_STATUS_OK:
        return STATUS_SUCCESS;
    case UACPI_STATUS_OUT_OF_MEMORY:
        return STATUS_INSUFFICIENT_RESOURCES;
    case UACPI_STATUS_INVALID_ARGUMENT:
        return STATUS_INVALID_PARAMETER;
    case UACPI_STATUS_ALREADY_EXISTS:
        return STATUS_OBJECT_NAME_COLLISION;
    case UACPI_STATUS_NOT_FOUND:
        return STATUS_NOT_FOUND;
    case UACPI_STATUS_UNIMPLEMENTED:
        return STATUS_NOT_SUPPORTED;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

static
uacpi_interrupt_ret
AcpiNewGpeHandler(
    _In_ uacpi_handle ctx,
    _In_ uacpi_namespace_node *gpe_device,
    _In_ uacpi_u16 idx)
{
    PACPI_NEW_GPE_CONTEXT gpeCtx = (PACPI_NEW_GPE_CONTEXT)ctx;
    BOOLEAN handled = FALSE;

    UNREFERENCED_PARAMETER(gpe_device);
    UNREFERENCED_PARAMETER(idx);

    if (gpeCtx && gpeCtx->ServiceRoutine)
        handled = gpeCtx->ServiceRoutine((PVOID)gpeCtx, gpeCtx->ServiceContext);

    return (handled ? UACPI_INTERRUPT_HANDLED : UACPI_INTERRUPT_NOT_HANDLED) | UACPI_GPE_REENABLE;
}

static
uacpi_gpe_triggering
AcpiNewModeToTriggering(_In_ KINTERRUPT_MODE Mode)
{
    return (Mode == LevelSensitive) ? UACPI_GPE_TRIGGERING_LEVEL : UACPI_GPE_TRIGGERING_EDGE;
}

static
NTSTATUS
NTAPI
AcpiNewGpeConnectVector(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG GpeNumber,
    _In_ KINTERRUPT_MODE Mode,
    _In_ BOOLEAN Shareable,
    _In_ PGPE_SERVICE_ROUTINE ServiceRoutine,
    _In_ PVOID ServiceContext,
    _Out_ PVOID *ObjectContext)
{
    PACPI_NEW_GPE_CONTEXT gpeCtx;
    uacpi_status st;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Shareable);

    if (ObjectContext)
        *ObjectContext = NULL;

    if (!ServiceRoutine || !ObjectContext)
        return STATUS_INVALID_PARAMETER;

    if (GpeNumber > 0xFFFF)
        return STATUS_INVALID_PARAMETER;

    gpeCtx = (PACPI_NEW_GPE_CONTEXT)ExAllocatePoolWithTag(NonPagedPool, sizeof(*gpeCtx), 'gAcu');
    if (!gpeCtx)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(gpeCtx, sizeof(*gpeCtx));
    gpeCtx->GpeNumber = GpeNumber;
    gpeCtx->Trigger = AcpiNewModeToTriggering(Mode);
    gpeCtx->ServiceRoutine = ServiceRoutine;
    gpeCtx->ServiceContext = ServiceContext;

    st = uacpi_install_gpe_handler(UACPI_NULL, (uacpi_u16)GpeNumber, gpeCtx->Trigger, AcpiNewGpeHandler, (uacpi_handle)gpeCtx);
    if (uacpi_unlikely_error(st))
    {
        ExFreePoolWithTag(gpeCtx, 'gAcu');
        return AcpiNewUacpiStatusToNtStatus(st);
    }

    *ObjectContext = (PVOID)gpeCtx;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
AcpiNewGpeDisconnectVector(
    _In_ PVOID ObjectContext)
{
    PACPI_NEW_GPE_CONTEXT gpeCtx = (PACPI_NEW_GPE_CONTEXT)ObjectContext;
    uacpi_status st;

    if (!gpeCtx)
        return STATUS_INVALID_PARAMETER;

    st = uacpi_uninstall_gpe_handler(UACPI_NULL, (uacpi_u16)gpeCtx->GpeNumber, AcpiNewGpeHandler);
    if (uacpi_unlikely_error(st))
        return AcpiNewUacpiStatusToNtStatus(st);

    ExFreePoolWithTag(gpeCtx, 'gAcu');
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
AcpiNewGpeEnableEvent(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID ObjectContext)
{
    PACPI_NEW_GPE_CONTEXT gpeCtx = (PACPI_NEW_GPE_CONTEXT)ObjectContext;
    uacpi_status st;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!gpeCtx)
        return STATUS_INVALID_PARAMETER;

    st = uacpi_enable_gpe(UACPI_NULL, (uacpi_u16)gpeCtx->GpeNumber);
    return AcpiNewUacpiStatusToNtStatus(st);
}

static
NTSTATUS
NTAPI
AcpiNewGpeDisableEvent(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID ObjectContext)
{
    PACPI_NEW_GPE_CONTEXT gpeCtx = (PACPI_NEW_GPE_CONTEXT)ObjectContext;
    uacpi_status st;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!gpeCtx)
        return STATUS_INVALID_PARAMETER;

    st = uacpi_disable_gpe(UACPI_NULL, (uacpi_u16)gpeCtx->GpeNumber);
    return AcpiNewUacpiStatusToNtStatus(st);
}

static
NTSTATUS
NTAPI
AcpiNewGpeClearStatus(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID ObjectContext)
{
    PACPI_NEW_GPE_CONTEXT gpeCtx = (PACPI_NEW_GPE_CONTEXT)ObjectContext;
    uacpi_status st;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!gpeCtx)
        return STATUS_INVALID_PARAMETER;

    st = uacpi_clear_gpe(UACPI_NULL, (uacpi_u16)gpeCtx->GpeNumber);
    return AcpiNewUacpiStatusToNtStatus(st);
}

static
NTSTATUS
NTAPI
AcpiNewGpeConnectVector2(
    _In_ PVOID Context,
    _In_ ULONG GpeNumber,
    _In_ KINTERRUPT_MODE Mode,
    _In_ BOOLEAN Shareable,
    _In_ PGPE_SERVICE_ROUTINE ServiceRoutine,
    _In_ PVOID ServiceContext,
    _Out_ PVOID *ObjectContext)
{
    return AcpiNewGpeConnectVector((PDEVICE_OBJECT)Context,
                                  GpeNumber,
                                  Mode,
                                  Shareable,
                                  ServiceRoutine,
                                  ServiceContext,
                                  ObjectContext);
}

static
NTSTATUS
NTAPI
AcpiNewGpeDisconnectVector2(
    _In_ PVOID Context,
    _In_ PVOID ObjectContext)
{
    UNREFERENCED_PARAMETER(Context);
    return AcpiNewGpeDisconnectVector(ObjectContext);
}

static
NTSTATUS
NTAPI
AcpiNewGpeEnableEvent2(
    _In_ PVOID Context,
    _In_ PVOID ObjectContext)
{
    return AcpiNewGpeEnableEvent((PDEVICE_OBJECT)Context, ObjectContext);
}

static
NTSTATUS
NTAPI
AcpiNewGpeDisableEvent2(
    _In_ PVOID Context,
    _In_ PVOID ObjectContext)
{
    return AcpiNewGpeDisableEvent((PDEVICE_OBJECT)Context, ObjectContext);
}

static
NTSTATUS
NTAPI
AcpiNewGpeClearStatus2(
    _In_ PVOID Context,
    _In_ PVOID ObjectContext)
{
    return AcpiNewGpeClearStatus((PDEVICE_OBJECT)Context, ObjectContext);
}

static
NTSTATUS
NTAPI
AcpiNewRegisterForDeviceNotifications(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDEVICE_NOTIFY_CALLBACK NotificationHandler,
    _In_ PVOID NotificationContext)
{
    PACPI_NEW_PDO_EXTENSION pdoExt;
    PACPI_NEW_NOTIFY_ENTRY entry;

    if (!DeviceObject || !AcpiNewIsPdo(DeviceObject) || !NotificationHandler)
        return STATUS_INVALID_PARAMETER;

    pdoExt = (PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension;

    entry = (PACPI_NEW_NOTIFY_ENTRY)ExAllocatePoolWithTag(NonPagedPool,
                                                          sizeof(*entry),
                                                          'nAcu');
    if (!entry)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(entry, sizeof(*entry));
    entry->V2 = FALSE;
    entry->Callback.V1 = NotificationHandler;
    entry->NotificationContext = NotificationContext;

    ExAcquireFastMutex(&pdoExt->NotifyLock);
    InsertTailList(&pdoExt->NotifyList, &entry->Link);
    ExReleaseFastMutex(&pdoExt->NotifyLock);

    return STATUS_SUCCESS;
}

static
VOID
NTAPI
AcpiNewUnregisterForDeviceNotifications(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PDEVICE_NOTIFY_CALLBACK NotificationHandler)
{
    PACPI_NEW_PDO_EXTENSION pdoExt;
    PLIST_ENTRY link, next;

    if (!DeviceObject || !AcpiNewIsPdo(DeviceObject) || !NotificationHandler)
        return;

    pdoExt = (PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension;

    ExAcquireFastMutex(&pdoExt->NotifyLock);
    link = pdoExt->NotifyList.Flink;
    while (link != &pdoExt->NotifyList)
    {
        PACPI_NEW_NOTIFY_ENTRY entry = CONTAINING_RECORD(link, ACPI_NEW_NOTIFY_ENTRY, Link);
        next = link->Flink;

        if (!entry->V2 && entry->Callback.V1 == NotificationHandler)
        {
            RemoveEntryList(link);
            ExFreePoolWithTag(entry, 'nAcu');
        }

        link = next;
    }
    ExReleaseFastMutex(&pdoExt->NotifyLock);
}

static
NTSTATUS
NTAPI
AcpiNewRegisterForDeviceNotifications2(
    _In_ PVOID Context,
    _In_ PDEVICE_NOTIFY_CALLBACK2 NotificationHandler,
    _In_ PVOID NotificationContext)
{
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    PACPI_NEW_PDO_EXTENSION pdoExt;
    PACPI_NEW_NOTIFY_ENTRY entry;

    if (!DeviceObject || !AcpiNewIsPdo(DeviceObject) || !NotificationHandler)
        return STATUS_INVALID_PARAMETER;

    pdoExt = (PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension;

    entry = (PACPI_NEW_NOTIFY_ENTRY)ExAllocatePoolWithTag(NonPagedPool,
                                                          sizeof(*entry),
                                                          'nAcu');
    if (!entry)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(entry, sizeof(*entry));
    entry->V2 = TRUE;
    entry->Callback.V2Cb = NotificationHandler;
    entry->NotificationContext = NotificationContext;

    ExAcquireFastMutex(&pdoExt->NotifyLock);
    InsertTailList(&pdoExt->NotifyList, &entry->Link);
    ExReleaseFastMutex(&pdoExt->NotifyLock);

    return STATUS_SUCCESS;
}

static
VOID
NTAPI
AcpiNewUnregisterForDeviceNotifications2(_In_ PVOID Context)
{
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;
    PACPI_NEW_PDO_EXTENSION pdoExt;
    PLIST_ENTRY link;

    if (!DeviceObject || !AcpiNewIsPdo(DeviceObject))
        return;

    pdoExt = (PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension;

    ExAcquireFastMutex(&pdoExt->NotifyLock);
    while (!IsListEmpty(&pdoExt->NotifyList))
    {
        PACPI_NEW_NOTIFY_ENTRY entry;
        link = RemoveHeadList(&pdoExt->NotifyList);
        entry = CONTAINING_RECORD(link, ACPI_NEW_NOTIFY_ENTRY, Link);
        ExFreePoolWithTag(entry, 'nAcu');
    }
    ExReleaseFastMutex(&pdoExt->NotifyLock);
}

NTSTATUS
AcpiNewPdoQueryInterface(_In_ PACPI_NEW_PDO_EXTENSION PdoExt, _In_ PIRP Irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    if (!PdoExt || !irpSp)
        return STATUS_INVALID_PARAMETER;

    if (RtlCompareMemory(irpSp->Parameters.QueryInterface.InterfaceType,
                         &GUID_ACPI_INTERFACE_STANDARD,
                         sizeof(GUID)) != sizeof(GUID))
    {
        return STATUS_NOT_SUPPORTED;
    }

    switch (irpSp->Parameters.QueryInterface.Version)
    {
    case 1:
    {
        PACPI_INTERFACE_STANDARD iface;
        if (irpSp->Parameters.QueryInterface.Size < sizeof(ACPI_INTERFACE_STANDARD))
            return STATUS_BUFFER_TOO_SMALL;

        iface = (PACPI_INTERFACE_STANDARD)irpSp->Parameters.QueryInterface.Interface;
        iface->Size = sizeof(ACPI_INTERFACE_STANDARD);
        iface->Version = 1;
        iface->Context = PdoExt->Common.Self;
        iface->InterfaceReference = AcpiNewInterfaceReference;
        iface->InterfaceDereference = AcpiNewInterfaceDereference;
        iface->GpeConnectVector = AcpiNewGpeConnectVector;
        iface->GpeDisconnectVector = AcpiNewGpeDisconnectVector;
        iface->GpeEnableEvent = AcpiNewGpeEnableEvent;
        iface->GpeDisableEvent = AcpiNewGpeDisableEvent;
        iface->GpeClearStatus = AcpiNewGpeClearStatus;
        iface->RegisterForDeviceNotifications = AcpiNewRegisterForDeviceNotifications;
        iface->UnregisterForDeviceNotifications = AcpiNewUnregisterForDeviceNotifications;

        AcpiNewInterfaceReference(PdoExt->Common.Self);
        status = STATUS_SUCCESS;
        break;
    }
    case 2:
    {
        PACPI_INTERFACE_STANDARD2 iface;
        if (irpSp->Parameters.QueryInterface.Size < sizeof(ACPI_INTERFACE_STANDARD2))
            return STATUS_BUFFER_TOO_SMALL;

        iface = (PACPI_INTERFACE_STANDARD2)irpSp->Parameters.QueryInterface.Interface;
        iface->Size = sizeof(ACPI_INTERFACE_STANDARD2);
        iface->Version = 2;
        iface->Context = PdoExt->Common.Self;
        iface->InterfaceReference = AcpiNewInterfaceReference;
        iface->InterfaceDereference = AcpiNewInterfaceDereference;
        iface->GpeConnectVector = AcpiNewGpeConnectVector2;
        iface->GpeDisconnectVector = AcpiNewGpeDisconnectVector2;
        iface->GpeEnableEvent = AcpiNewGpeEnableEvent2;
        iface->GpeDisableEvent = AcpiNewGpeDisableEvent2;
        iface->GpeClearStatus = AcpiNewGpeClearStatus2;
        iface->RegisterForDeviceNotifications = AcpiNewRegisterForDeviceNotifications2;
        iface->UnregisterForDeviceNotifications = AcpiNewUnregisterForDeviceNotifications2;

        AcpiNewInterfaceReference(PdoExt->Common.Self);
        status = STATUS_SUCCESS;
        break;
    }
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    Irp->IoStatus.Status = status;
    return status;
}

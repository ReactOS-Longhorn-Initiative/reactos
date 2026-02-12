#include "precomp.h"
#include "acpi_new.h"

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
AcpiNewDeliverNotify(
    _In_ PACPI_NEW_FDO_EXTENSION FdoExt,
    _In_ uacpi_namespace_node *Node,
    _In_ ULONG NotifyCode)
{
    PLIST_ENTRY entry;
    PACPI_NEW_PDO_EXTENSION pdoExt = NULL;

    if (!FdoExt || !Node)
        return;

    ExAcquireFastMutex(&FdoExt->Mutex);
    for (entry = FdoExt->PdoList.Flink; entry != &FdoExt->PdoList; entry = entry->Flink)
    {
        PACPI_NEW_PDO_EXTENSION candidate = CONTAINING_RECORD(entry, ACPI_NEW_PDO_EXTENSION, Link);
        if (candidate->Present && candidate->Node == Node)
        {
            pdoExt = candidate;
            ObReferenceObject(pdoExt->Common.Self);
            break;
        }
    }
    ExReleaseFastMutex(&FdoExt->Mutex);

    if (!pdoExt)
        return;

    ExAcquireFastMutex(&pdoExt->NotifyLock);
    for (entry = pdoExt->NotifyList.Flink; entry != &pdoExt->NotifyList; entry = entry->Flink)
    {
        PACPI_NEW_NOTIFY_ENTRY n = CONTAINING_RECORD(entry, ACPI_NEW_NOTIFY_ENTRY, Link);
        if (n->V2)
        {
            if (n->Callback.V2Cb)
                n->Callback.V2Cb(n->NotificationContext, NotifyCode);
        }
        else
        {
            if (n->Callback.V1)
                n->Callback.V1(n->NotificationContext, NotifyCode);
        }
    }
    ExReleaseFastMutex(&pdoExt->NotifyLock);

    ObDereferenceObject(pdoExt->Common.Self);
}

static
VOID
NTAPI
AcpiNewNotifyWorkItemRoutine(_In_ PVOID Parameter)
{
    PACPI_NEW_FDO_EXTENSION fdoExt = (PACPI_NEW_FDO_EXTENSION)Parameter;
    if (!fdoExt)
        return;

    if (!fdoExt->Removed && fdoExt->PhysicalDeviceObject)
        IoInvalidateDeviceRelations(fdoExt->PhysicalDeviceObject, BusRelations);

    InterlockedExchange(&fdoExt->NotifyWorkQueued, 0);
}

uacpi_status
AcpiNewNotifyHandler(
    _In_ uacpi_handle context,
    _In_ uacpi_namespace_node *node,
    _In_ uacpi_u64 value)
{
    PACPI_NEW_FDO_EXTENSION fdoExt = (PACPI_NEW_FDO_EXTENSION)context;
    ULONG notifyCode = (ULONG)value;

    if (!fdoExt || fdoExt->Removed)
        return UACPI_STATUS_OK;

    if (node)
        AcpiNewDeliverNotify(fdoExt, node, notifyCode);

    InterlockedExchange(&fdoExt->EnumerationDirty, 1);

    if (InterlockedCompareExchange(&fdoExt->NotifyWorkQueued, 1, 0) == 0)
    {
        ExInitializeWorkItem(&fdoExt->NotifyWorkItem, AcpiNewNotifyWorkItemRoutine, fdoExt);
        ExQueueWorkItem(&fdoExt->NotifyWorkItem, DelayedWorkQueue);
    }

    return UACPI_STATUS_OK;
}

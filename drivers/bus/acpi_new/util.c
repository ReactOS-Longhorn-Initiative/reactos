#include "precomp.h"
#include "acpi_new.h"

typedef struct _ACPI_NEW_SYNC_IRP_CONTEXT
{
    KEVENT Event;
    NTSTATUS Status;
} ACPI_NEW_SYNC_IRP_CONTEXT, *PACPI_NEW_SYNC_IRP_CONTEXT;

static
NTSTATUS
NTAPI
AcpiNewSyncCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID Context)
{
    PACPI_NEW_SYNC_IRP_CONTEXT sync = (PACPI_NEW_SYNC_IRP_CONTEXT)Context;
    UNREFERENCED_PARAMETER(DeviceObject);

    sync->Status = Irp->IoStatus.Status;
    KeSetEvent(&sync->Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
AcpiNewForwardIrpSynchronously(
    _In_ PACPI_NEW_FDO_EXTENSION FdoExt,
    _In_ PIRP Irp)
{
    ACPI_NEW_SYNC_IRP_CONTEXT sync;
    NTSTATUS status;

    KeInitializeEvent(&sync.Event, NotificationEvent, FALSE);
    sync.Status = STATUS_UNSUCCESSFUL;

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, AcpiNewSyncCompletion, &sync, TRUE, TRUE, TRUE);
    status = IoCallDriver(FdoExt->LowerDevice, Irp);

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&sync.Event, Executive, KernelMode, FALSE, NULL);
        status = sync.Status;
    }
    return status;
}

PWSTR
AcpiNewDupUnicodeString(_In_reads_(Chars) const WCHAR *Src, _In_ SIZE_T Chars)
{
    SIZE_T bytes = (Chars + 1) * sizeof(WCHAR);
    PWSTR dst = (PWSTR)ExAllocatePoolWithTag(PagedPool, bytes, 'dAcu');
    if (!dst)
        return NULL;
    RtlCopyMemory(dst, Src, Chars * sizeof(WCHAR));
    dst[Chars] = UNICODE_NULL;
    return dst;
}

PWSTR
AcpiNewDupMultiSz(_In_z_ const WCHAR *Src)
{
    SIZE_T chars = 0;
    const WCHAR *p = Src;

    if (!Src)
        return NULL;

    for (;;)
    {
        SIZE_T l = wcslen(p);
        chars += (l + 1);
        if (l == 0)
            break;
        p += (l + 1);
    }

    {
        SIZE_T bytes = chars * sizeof(WCHAR);
        PWSTR dst = (PWSTR)ExAllocatePoolWithTag(PagedPool, bytes, 'mAcu');
        if (!dst)
            return NULL;
        RtlCopyMemory(dst, Src, bytes);
        return dst;
    }
}

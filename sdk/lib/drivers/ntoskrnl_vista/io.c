/*
 * PROJECT:     ReactOS Kernel - Vista+ APIs
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Io functions of Vista+
 * COPYRIGHT:   2016 Pierre Schweitzer (pierre@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>

typedef struct _EX_WORKITEM_CONTEXT
{
    PIO_WORKITEM WorkItem;
    PIO_WORKITEM_ROUTINE_EX WorkItemRoutineEx;
    PVOID Context;
} EX_WORKITEM_CONTEXT, *PEX_WORKITEM_CONTEXT;

#define TAG_IOWI 'IWOI'

extern
NTSTATUS
NTAPI
IopSynchronousCompletion(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp,
                         IN PVOID Context);

NTKRNLVISTAAPI
NTSTATUS
NTAPI
IoCreateArcName(
  _In_ PDEVICE_OBJECT DeviceObject
)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTKRNLVISTAAPI
NTSTATUS
NTAPI
IoGetIrpExtraCreateParameter(IN PIRP Irp,
                             OUT PECP_LIST *ExtraCreateParameter)
{
    /* Check we have a create operation */
    if (!BooleanFlagOn(Irp->Flags, IRP_CREATE_OPERATION))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If so, return user buffer */
    *ExtraCreateParameter = Irp->UserBuffer;
    return STATUS_SUCCESS;
}

_Function_class_(IO_WORKITEM_ROUTINE)
static
VOID
NTAPI
IopWorkItemExCallback(
    PDEVICE_OBJECT DeviceObject,
    PVOID Ctx)
{
    PEX_WORKITEM_CONTEXT context = Ctx;

    context->WorkItemRoutineEx(DeviceObject, context->Context, context->WorkItem);
    ExFreePoolWithTag(context, TAG_IOWI);
}

NTKRNLVISTAAPI
VOID
NTAPI
IoQueueWorkItemEx(
    _Inout_ PIO_WORKITEM IoWorkItem,
    _In_ PIO_WORKITEM_ROUTINE_EX WorkerRoutine,
    _In_ WORK_QUEUE_TYPE QueueType,
    _In_opt_ __drv_aliasesMem PVOID Context)
{
    PEX_WORKITEM_CONTEXT newContext = ExAllocatePoolWithTag(NonPagedPoolMustSucceed, sizeof(*newContext), TAG_IOWI);
    newContext->WorkItem = IoWorkItem;
    newContext->WorkItemRoutineEx = WorkerRoutine;
    newContext->Context = Context;

    IoQueueWorkItem(IoWorkItem, IopWorkItemExCallback, QueueType, newContext);
}

NTSTATUS
NTAPI
IoAllocateSfioStreamIdentifier(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG Length,
  _In_ PVOID Signature,
  _Out_ PVOID *StreamIdentifier)
{
    UNIMPLEMENTED;
    *StreamIdentifier = NULL;
    return STATUS_SUCCESS;
}


PVOID
NTAPI
IoGetSfioStreamIdentifier(
    _In_ PFILE_OBJECT FileObject,
    _In_ PVOID Signature)
{
    UNIMPLEMENTED;
    return NULL;
}

NTSTATUS
NTAPI
IoFreeSfioStreamIdentifier(
    _In_ PFILE_OBJECT FileObject,
    _In_ PVOID Signature)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTKRNLVISTAAPI
NTSTATUS
NTAPI
IoSetActivityIdIrp(
    _In_ PIRP    Irp,
    _In_opt_ LPCGUID Guid
)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKRNLVISTAAPI
NTSTATUS
NTAPI
IoSetDevicePropertyData(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _In_ ULONG Flags,
    _In_ DEVPROPTYPE Type,
    _In_ ULONG Size,
    _In_opt_ PVOID Data)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKRNLVISTAAPI
NTSTATUS
NTAPI
IoGetDevicePropertyData(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _Reserved_ ULONG Flags,
    _In_ ULONG Size,
    _Out_ PVOID Data,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTKRNLVISTAAPI
NTSTATUS
IoSetDeviceInterfacePropertyData(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _In_ ULONG Flags,
    _In_ DEVPROPTYPE Type,
    _In_ ULONG Size,
    _In_reads_bytes_opt_(Size) PVOID Data)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTKRNLVISTAAPI
IO_PRIORITY_HINT
NTAPI
IoGetIoPriorityHint(
    _In_ PIRP Irp)
{
    return IoPriorityNormal;
}

NTKRNLVISTAAPI
VOID
IoSetMasterIrpStatus(
    _Inout_ PIRP MasterIrp,
    _In_ NTSTATUS Status)
{
    NTSTATUS MasterStatus = MasterIrp->IoStatus.Status;

    if (Status == STATUS_FT_READ_FROM_COPY)
    {
        return;
    }

    if ((Status == STATUS_VERIFY_REQUIRED) ||
        (MasterStatus == STATUS_SUCCESS && !NT_SUCCESS(Status)) ||
        (!NT_SUCCESS(MasterStatus) && !NT_SUCCESS(Status) && Status > MasterStatus))
    {
        MasterIrp->IoStatus.Status = Status;
    }
}

NTKRNLVISTAAPI
NTSTATUS
NTAPI
IoGetActivityIdIrp(
    _In_ PIRP    Irp,
    _Out_ LPCGUID Guid
)
{
    UNIMPLEMENTED;
    return STATUS_NOT_FOUND;
}

NTSTATUS
NTAPI
IoGetAffinityInterrupt(
  _In_ PKINTERRUPT InterruptObject,
  _Out_ PGROUP_AFFINITY GroupAffinity)
{
    UNIMPLEMENTED;
    return STATUS_INVALID_PARAMETER;
}

NTKRNLVISTAAPI
VOID
NTAPI
IoReportInterruptActive(
    _In_ PIO_REPORT_INTERRUPT_ACTIVE_STATE_PARAMETERS Parameters
)
{
    UNIMPLEMENTED;
}

NTKRNLVISTAAPI
VOID
NTAPI
IoReportInterruptInactive(
    _In_ PIO_REPORT_INTERRUPT_ACTIVE_STATE_PARAMETERS Parameters
)
{
    UNIMPLEMENTED;
}

LONG_PTR FASTCALL
ObfReferenceObjectWithTag (
    PVOID Object,
    ULONG Tag )
{
    return ObfReferenceObject(Object);
}


LONG_PTR FASTCALL
ObfDereferenceObjectWithTag (
    PVOID Object,
    ULONG Tag )
{
    return ObfDereferenceObject(Object);
}

errno_t memcpy_s(
    void* dest,
    size_t      destSize,
    const void* src,
    size_t      count
)
{
    if (dest == NULL) {
        return EINVAL;
    }
    
    if (destSize == 0) {
        return ERANGE;
    }

    if (src == NULL || count == 0) {
        RtlZeroMemory(dest, destSize); 
        return (src == NULL) ? EINVAL : 0;
    }

    if (count > destSize) {
        RtlZeroMemory(dest, destSize);
        return ERANGE;
    }
    const unsigned char* s = (const unsigned char*)src;
    unsigned char* d = (unsigned char*)dest;

    if ((d >= s && d < s + count) || (s >= d && s < d + destSize)) {
        RtlZeroMemory(dest, destSize);
        return EINVAL; 
    }
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }

    return 0; // Success
}

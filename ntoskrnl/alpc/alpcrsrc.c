/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC resource reserves (pre-allocated message quota)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/*
 * A resource-reserve handle is returned to user mode tagged with the high bit.
 * A send whose PORT_MESSAGE.MessageId carries such a (negative) value is treated
 * as a reserve-backed send (see AlpcpSendRequest). The low 31 bits index the
 * port's handle table.
 */
#define ALPC_RESERVE_HANDLE_TAG 0x80000000

/* Largest message a reserve may back (one PORT_MESSAGE below the ABI maximum). */
#define ALPC_RESERVE_MAX_MESSAGE_SIZE 0xFFD7

/* FUNCTIONS ******************************************************************/

/**
 * @brief Claims a reserve referenced by a tagged handle on a sending port.
 *
 * Called from the send path when a message carries a reserve handle (its
 * PORT_MESSAGE.MessageId has the high bit set). The reserve is looked up in the
 * port's handle table and its Active flag is claimed atomically: a reserve can
 * only back one in-flight send, so a second attempt fails. Mirrors the reserve
 * branch of AlpcpSendMessage.
 *
 * @return STATUS_SUCCESS once the reserve is claimed, STATUS_OBJECTID_NOT_FOUND
 * if the handle does not resolve, or STATUS_RESOURCE_IN_USE if already active.
 */
NTSTATUS
NTAPI
AlpcpConsumeReserve(
    _In_ PALPC_PORT Port,
    _In_ ULONG TaggedHandle)
{
    PKALPC_RESERVE Reserve;

    if (Port->CommunicationInfo == NULL)
        return STATUS_OBJECTID_NOT_FOUND;

    Reserve = AlpcpReferenceHandle(&Port->CommunicationInfo->HandleTable,
                                   (ALPC_HANDLE)(ULONG_PTR)(TaggedHandle & ~ALPC_RESERVE_HANDLE_TAG));
    if (Reserve == NULL)
        return STATUS_OBJECTID_NOT_FOUND;

    if (InterlockedCompareExchange(&Reserve->Active, 1, 0) != 0)
        return STATUS_RESOURCE_IN_USE;

    return STATUS_SUCCESS;
}

/**
 * @brief Reserves send quota for a future message on a port.
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcCreateResourceReserve(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_ SIZE_T MessageSize,
    _Out_ PALPC_HANDLE ResourceId)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_RESERVE Reserve;
    ALPC_HANDLE Handle;
    NTSTATUS Status;

    /* Flags must be zero; the message size cannot exceed what one message can
     * carry. Both verified against the Win11 oracle. */
    if (Flags != 0)
        return STATUS_INVALID_PARAMETER;
    if (MessageSize > ALPC_RESERVE_MAX_MESSAGE_SIZE)
        return STATUS_BUFFER_OVERFLOW;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Reserve = ExAllocatePoolWithTag(NonPagedPool, sizeof(KALPC_RESERVE), TAG_ALPC_HANDLE);
    if (Reserve == NULL)
    {
        ObDereferenceObject(Port);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Reserve, sizeof(KALPC_RESERVE));
    Reserve->OwnerPort = Port;
    Reserve->HandleTable = &Port->CommunicationInfo->HandleTable;
    Reserve->Active = 0;

    Handle = AlpcpInsertHandle(&Port->CommunicationInfo->HandleTable, Reserve);
    if (Handle == NULL)
    {
        ExFreePoolWithTag(Reserve, TAG_ALPC_HANDLE);
        ObDereferenceObject(Port);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Reserve->Handle = Handle;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForWrite(ResourceId, sizeof(ALPC_HANDLE), sizeof(ALPC_HANDLE));
        /* Hand the caller the tagged (negative) form of the handle. */
        *ResourceId = (ALPC_HANDLE)((ULONG_PTR)Handle | ALPC_RESERVE_HANDLE_TAG);
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ObDereferenceObject(Port);
    return Status;
}

/**
 * @brief Releases a previously created resource reserve.
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcDeleteResourceReserve(
    _In_ HANDLE PortHandle,
    _In_ ULONG Flags,
    _In_ ALPC_HANDLE ResourceId)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PKALPC_RESERVE Reserve;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Strip the high-bit tag the create call added before indexing the table. */
    Reserve = AlpcpRemoveHandle(&Port->CommunicationInfo->HandleTable,
                                (ALPC_HANDLE)((ULONG_PTR)ResourceId & ~(ULONG_PTR)ALPC_RESERVE_HANDLE_TAG));
    if (Reserve == NULL)
    {
        ObDereferenceObject(Port);
        return STATUS_INVALID_HANDLE;
    }

    ExFreePoolWithTag(Reserve, TAG_ALPC_HANDLE);
    ObDereferenceObject(Port);
    return STATUS_SUCCESS;
}

/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC per-message queries and sender identification
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS ********************************************************/

/**
 * @brief Locates a received (pending) message by id and returns the sender's
 *        client id, plus a reference to its waiting thread if it has one.
 *
 * Received messages - synchronous requests and datagrams alike - sit in the
 * connection port's pending queue. A synchronous request also records the thread
 * blocked on the reply; a datagram has none.
 *
 * @return TRUE if a matching message was found.
 */
static
BOOLEAN
AlpcpFindPendingSender(
    _In_ PALPC_PORT Port,
    _In_ ULONG MessageId,
    _Out_ PCLIENT_ID ClientId,
    _Outptr_result_maybenull_ PETHREAD *Thread)
{
    BOOLEAN Found = FALSE;
    PLIST_ENTRY Entry;

    *Thread = NULL;
    ClientId->UniqueProcess = NULL;
    ClientId->UniqueThread = NULL;

    KeAcquireGuardedMutex(&AlpcpLock);
    for (Entry = Port->PendingQueue.Flink;
         Entry != &Port->PendingQueue;
         Entry = Entry->Flink)
    {
        PKALPC_MESSAGE Message = CONTAINING_RECORD(Entry, KALPC_MESSAGE, Entry);
        if (Message->PortMessage.MessageId == MessageId)
        {
            *ClientId = Message->PortMessage.ClientId;
            if (Message->WaitingThread != NULL)
            {
                *Thread = Message->WaitingThread;
                ObReferenceObject(*Thread);
            }
            Found = TRUE;
            break;
        }
    }
    KeReleaseGuardedMutex(&AlpcpLock);

    return Found;
}

/**
 * @brief Returns the sender's SID for a received message.
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
static
NTSTATUS
AlpcpQuerySidMessage(
    _In_ PALPC_PORT Port,
    _In_ ULONG MessageId,
    _Out_ PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    CLIENT_ID ClientId;
    PETHREAD Thread;
    PEPROCESS Process;
    PACCESS_TOKEN Token;
    PTOKEN_USER TokenUserInfo;
    ULONG SidLength;
    NTSTATUS Status;

    if (!AlpcpFindPendingSender(Port, MessageId, &ClientId, &Thread))
        return STATUS_REQUEST_CANCELED;

    if (Thread != NULL)
        ObDereferenceObject(Thread);

    Status = PsLookupProcessByProcessId(ClientId.UniqueProcess, &Process);
    if (!NT_SUCCESS(Status))
        return STATUS_REQUEST_CANCELED;

    Token = PsReferencePrimaryToken(Process);
    Status = SeQueryInformationToken(Token, TokenUser, (PVOID *)&TokenUserInfo);
    PsDereferencePrimaryToken(Token);
    ObDereferenceObject(Process);
    if (!NT_SUCCESS(Status))
        return Status;

    SidLength = RtlLengthSid(TokenUserInfo->User.Sid);

    _SEH2_TRY
    {
        if (ReturnLength != NULL)
        {
            if (PreviousMode != KernelMode)
                ProbeForWriteUlong(ReturnLength);
            *ReturnLength = SidLength;
        }

        if (Length < SidLength)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            if (PreviousMode != KernelMode)
                ProbeForWrite(Buffer, SidLength, sizeof(ULONG));
            RtlCopySid(SidLength, Buffer, TokenUserInfo->User.Sid);
            Status = STATUS_SUCCESS;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ExFreePool(TokenUserInfo);
    return Status;
}

/* PUBLIC FUNCTIONS ********************************************************/

/**
 * @brief Queries information about a received message (sender SID, ...).
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcQueryInformationMessage(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE PortMessage,
    _In_ ALPC_MESSAGE_INFORMATION_CLASS MessageInformationClass,
    _Out_opt_ PVOID MessageInformation,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    ULONG MessageId;
    NTSTATUS Status;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(PortMessage, sizeof(PORT_MESSAGE), sizeof(ULONG));
        MessageId = PortMessage->MessageId;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (MessageInformationClass)
    {
        case AlpcMessageSidInformation:
            Status = AlpcpQuerySidMessage(Port, MessageId, MessageInformation, Length,
                                          ReturnLength, PreviousMode);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    ObDereferenceObject(Port);
    return Status;
}

/**
 * @brief Opens the process that sent a message.
 *
 * Works for datagrams as well as synchronous requests: the sender process is
 * resolved from the message's recorded client id.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcOpenSenderProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE PortMessage,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PETHREAD Thread;
    PEPROCESS Process;
    CLIENT_ID ClientId;
    ULONG MessageId;
    HANDLE Handle;
    NTSTATUS Status;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
        {
            ProbeForWriteHandle(ProcessHandle);
            ProbeForRead(PortMessage, sizeof(PORT_MESSAGE), sizeof(ULONG));
        }
        MessageId = PortMessage->MessageId;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    if (!AlpcpFindPendingSender(Port, MessageId, &ClientId, &Thread))
    {
        ObDereferenceObject(Port);
        return STATUS_REQUEST_CANCELED;
    }
    if (Thread != NULL)
        ObDereferenceObject(Thread);
    ObDereferenceObject(Port);

    Status = PsLookupProcessByProcessId(ClientId.UniqueProcess, &Process);
    if (!NT_SUCCESS(Status))
        return STATUS_REQUEST_CANCELED;

    /* Access is authorized by possession of the pending message. */
    Status = ObOpenObjectByPointer(Process, 0, NULL, DesiredAccess, PsProcessType,
                                   KernelMode, &Handle);
    ObDereferenceObject(Process);
    if (!NT_SUCCESS(Status))
        return Status;

    _SEH2_TRY
    {
        *ProcessHandle = Handle;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ObCloseHandle(Handle, PreviousMode);
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

/**
 * @brief Opens the thread that sent a synchronous request.
 *
 * Only a synchronous request has a waiting sender thread to open. For a datagram
 * the result is STATUS_ACCESS_DENIED if the sender is still alive, or
 * STATUS_REQUEST_CANCELED if it has exited.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcOpenSenderThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE PortMessage,
    _In_ ULONG Flags,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PETHREAD Thread;
    PEPROCESS Process;
    CLIENT_ID ClientId;
    ULONG MessageId;
    HANDLE Handle;
    NTSTATUS Status;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
        {
            ProbeForWriteHandle(ThreadHandle);
            ProbeForRead(PortMessage, sizeof(PORT_MESSAGE), sizeof(ULONG));
        }
        MessageId = PortMessage->MessageId;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    if (!AlpcpFindPendingSender(Port, MessageId, &ClientId, &Thread))
    {
        ObDereferenceObject(Port);
        return STATUS_REQUEST_CANCELED;
    }
    ObDereferenceObject(Port);

    if (Thread == NULL)
    {
        /* Datagram: no thread is waiting on this message. */
        Status = PsLookupProcessByProcessId(ClientId.UniqueProcess, &Process);
        if (NT_SUCCESS(Status))
        {
            ObDereferenceObject(Process);
            return STATUS_ACCESS_DENIED;
        }
        return STATUS_REQUEST_CANCELED;
    }

    Status = ObOpenObjectByPointer(Thread, 0, NULL, DesiredAccess, PsThreadType,
                                   KernelMode, &Handle);
    ObDereferenceObject(Thread);
    if (!NT_SUCCESS(Status))
        return Status;

    _SEH2_TRY
    {
        *ThreadHandle = Handle;
        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ObCloseHandle(Handle, PreviousMode);
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

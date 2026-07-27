/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC connection handshake (connect / accept)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

#define ALPC_PORT_TYPE_COMMUNICATION 2

/* The high bits the kernel folds into a received connection request type. */
#define ALPC_CONNECTION_REQUEST_TYPE (0x3000 | LPC_CONNECTION_REQUEST)

/* PRIVATE FUNCTIONS ********************************************************/

/**
 * @brief Creates and initializes a communication port (client or server side).
 *
 * The port inherits the connection port's maximum message length and is owned
 * by the calling process.
 */
NTSTATUS
NTAPI
AlpcpCreateCommunicationPort(
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PALPC_PORT ConnectionPort,
    _Outptr_ PALPC_PORT *OutPort)
{
    NTSTATUS Status;
    PALPC_PORT Port;
    PEPROCESS Process;

    Status = ObCreateObject(PreviousMode,
                            AlpcPortObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(ALPC_PORT),
                            0,
                            0,
                            (PVOID *)&Port);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlZeroMemory(Port, sizeof(ALPC_PORT));

    Status = AlpcpInitializePort(Port, ALPC_PORT_TYPE_COMMUNICATION, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Port);
        return Status;
    }

    Port->PortAttributes = ConnectionPort->PortAttributes;
    Port->PortAttributes.Flags &= 0x03FF0000;

    Process = PsGetCurrentProcess();
    ObReferenceObject(Process);
    Port->OwnerProcess = Process;

    /* Give the port its communication info (handle table + routing reference to
     * the connection port). */
    Status = AlpcpAllocateCommunicationInfo(Port, ConnectionPort);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Port);
        return Status;
    }

    *OutPort = Port;
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS ********************************************************/

/**
 * @brief Connects a client to a named ALPC connection port.
 *
 * Opens the named server port, creates the client communication port (left in
 * the ConnectionPending state), queues a connection request on the server's
 * main queue and — for a synchronous request — blocks on the calling thread's
 * ALPC wait semaphore until the server accepts or refuses.
 *
 * @return STATUS_SUCCESS once connected, STATUS_PORT_CONNECTION_REFUSED if the
 * server refused, STATUS_OBJECT_NAME_NOT_FOUND for an unknown port, or another
 * appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
    _In_ ULONG Flags,
    _In_opt_ PSID RequiredServerSid,
    _Inout_opt_ PPORT_MESSAGE ConnectionMessage,
    _Inout_opt_ PSIZE_T BufferLength,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES OutMessageAttributes,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES InMessageAttributes,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PETHREAD Thread = PsGetCurrentThread();
    UNICODE_STRING CapturedName;
    BOOLEAN NameCaptured = FALSE;
    PALPC_PORT ServerPort = NULL;
    PALPC_PORT ClientPort = NULL;
    PETHREAD Receiver;
    HANDLE ClientHandle = NULL;
    PKALPC_MESSAGE Message = NULL;
    LARGE_INTEGER CapturedTimeout;
    PLARGE_INTEGER TimeoutPtr = NULL;
    PORT_MESSAGE CapturedHeader;
    ULONG DataLength = 0;
    NTSTATUS Status;

    RtlZeroMemory(&CapturedHeader, sizeof(CapturedHeader));

    /* Probe the output handle and capture the in-parameters. */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(PortHandle);
            if (Timeout != NULL)
            {
                ProbeForRead(Timeout, sizeof(LARGE_INTEGER), sizeof(ULONG));
                CapturedTimeout = *Timeout;
                TimeoutPtr = &CapturedTimeout;
            }
            if (ConnectionMessage != NULL)
            {
                ProbeForRead(ConnectionMessage, sizeof(PORT_MESSAGE), sizeof(ULONG));
                CapturedHeader = *ConnectionMessage;
                DataLength = CapturedHeader.u1.s1.DataLength;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        if (Timeout != NULL)
        {
            CapturedTimeout = *Timeout;
            TimeoutPtr = &CapturedTimeout;
        }
        if (ConnectionMessage != NULL)
        {
            CapturedHeader = *ConnectionMessage;
            DataLength = CapturedHeader.u1.s1.DataLength;
        }
    }

    Status = ProbeAndCaptureUnicodeString(&CapturedName, PreviousMode, PortName);
    if (!NT_SUCCESS(Status))
        return Status;
    NameCaptured = TRUE;

    /* Open the named server connection port. The name is already captured into
     * kernel memory, so the lookup runs as KernelMode to avoid re-probing it. */
    Status = ObReferenceObjectByName(&CapturedName,
                                     OBJ_CASE_INSENSITIVE,
                                     NULL,
                                     0,
                                     AlpcPortObjectType,
                                     KernelMode,
                                     NULL,
                                     (PVOID *)&ServerPort);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    /* Create the client communication port and publish a handle for it. */
    Status = AlpcpCreateCommunicationPort(PreviousMode, NULL, ServerPort, &ClientPort);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    ClientPort->u1.ConnectionPending = TRUE;
    /* Routing info (incl. the reference to the connection port) was attached by
     * AlpcpCreateCommunicationPort. */

    Status = ObInsertObject(ClientPort, NULL, PORT_ALL_ACCESS, 0, NULL, &ClientHandle);
    if (!NT_SUCCESS(Status))
    {
        ClientPort = NULL; /* ObInsertObject dereferenced it. */
        goto Cleanup;
    }
    /* Keep our own reference while we drive the handshake. */
    ObReferenceObject(ClientPort);

    /* Build the connection request message. */
    Message = AlpcpAllocateMessage(DataLength);
    if (Message == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Message->PortMessage.u2.s2.Type = ALPC_CONNECTION_REQUEST_TYPE;
    Message->PortMessage.ClientId = Thread->Cid;
    Message->OwnerPort = ClientPort;
    Message->ConnectionPort = ServerPort;
    Message->WaitingThread = Thread;

    if (DataLength != 0 && ConnectionMessage != NULL)
    {
        _SEH2_TRY
        {
            RtlCopyMemory(AlpcpGetMessageData(Message),
                          (PUCHAR)ConnectionMessage + sizeof(PORT_MESSAGE),
                          DataLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            goto Cleanup;
        }
        _SEH2_END;
    }

    /* Arm the wait semaphore before queueing so the accept cannot race ahead
     * of us and lose the wakeup. */
    KeInitializeSemaphore(&Thread->AlpcWaitSemaphore, 0, MAXLONG);

    KeAcquireGuardedMutex(&AlpcpLock);
    ServerPort->PendingClientPort = ClientPort;
    ServerPort->CachedMessage = Message;
    ServerPort->CachedConnectionMessageId = Message->PortMessage.MessageId;
    InsertTailList(&ServerPort->MainQueue, &Message->Entry);
    ServerPort->MainQueueLength++;
    Message->u1.QueueType = 1;
    Receiver = AlpcpDequeueReceiver(ServerPort);
    KeReleaseGuardedMutex(&AlpcpLock);

    if (Receiver != NULL)
        KeReleaseSemaphore(&Receiver->AlpcWaitSemaphore, 0, 1, FALSE);

    if (Flags & ALPC_MSGFLG_SYNC_REQUEST)
    {
        Status = KeWaitForSingleObject(&Thread->AlpcWaitSemaphore,
                                       WrLpcReply,
                                       PreviousMode,
                                       FALSE,
                                       TimeoutPtr);
        if (Status == STATUS_SUCCESS)
        {
            if (ClientPort->u1.ConnectionRefused)
                Status = STATUS_PORT_CONNECTION_REFUSED;
            else if (!ClientPort->u1.ConnectionPending)
                Status = STATUS_SUCCESS;
            else
                Status = STATUS_PORT_CONNECTION_REFUSED;
        }
        else if (Status == STATUS_TIMEOUT)
        {
            Status = STATUS_TIMEOUT;
        }
    }
    else
    {
        /* Asynchronous connect returns the still-pending client port. */
        Status = STATUS_SUCCESS;
    }

    /* Reclaim the connection request and detach it from the server port. */
    KeAcquireGuardedMutex(&AlpcpLock);
    if (ServerPort->CachedMessage == Message)
    {
        ServerPort->CachedMessage = NULL;
        ServerPort->PendingClientPort = NULL;
    }
    if (Message->u1.QueueType == 1)
    {
        RemoveEntryList(&Message->Entry);
        ServerPort->MainQueueLength--;
        Message->u1.QueueType = 0;
    }
    KeReleaseGuardedMutex(&AlpcpLock);
    AlpcpFreeMessage(Message);
    Message = NULL;

    if (Status == STATUS_SUCCESS)
    {
        *PortHandle = ClientHandle;
        ClientHandle = NULL;
    }

Cleanup:
    if (Message != NULL)
        AlpcpFreeMessage(Message);
    if (ClientPort != NULL)
        ObDereferenceObject(ClientPort);
    if (ServerPort != NULL)
        ObDereferenceObject(ServerPort);
    if (ClientHandle != NULL)
        ObCloseHandle(ClientHandle, PreviousMode);
    if (NameCaptured)
        ReleaseCapturedUnicodeString(&CapturedName, PreviousMode);

    return Status;
}

/**
 * @brief Accepts or refuses a pending connection request on a server port.
 *
 * On acceptance a server communication port is created (carrying the supplied
 * port context), the client port leaves ConnectionPending, and the blocked
 * client thread is woken. On refusal the client port is marked refused.
 *
 * @return STATUS_SUCCESS on success, otherwise an appropriate NTSTATUS.
 */
NTSTATUS
NTAPI
NtAlpcAcceptConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ HANDLE ConnectionPortHandle,
    _In_ ULONG Flags,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes,
    _In_opt_ PVOID PortContext,
    _In_ PPORT_MESSAGE ConnectionRequest,
    _Inout_opt_ PALPC_MESSAGE_ATTRIBUTES ConnectionMessageAttributes,
    _In_ BOOLEAN AcceptConnection)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT ConnectionPort = NULL;
    PALPC_PORT ServerPort = NULL;
    PALPC_PORT ClientPort;
    PKALPC_MESSAGE Message;
    PETHREAD WaitingThread = NULL;
    HANDLE ServerHandle = NULL;
    ULONG RequestMessageId = 0;
    NTSTATUS Status;

    /* Probe the output handle and capture the request message id. */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            if (AcceptConnection)
                ProbeForWriteHandle(PortHandle);
            ProbeForRead(ConnectionRequest, sizeof(PORT_MESSAGE), sizeof(ULONG));
            RequestMessageId = ConnectionRequest->MessageId;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        RequestMessageId = ConnectionRequest->MessageId;
    }

    Status = ObReferenceObjectByHandle(ConnectionPortHandle,
                                       0,
                                       AlpcPortObjectType,
                                       PreviousMode,
                                       (PVOID *)&ConnectionPort,
                                       NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Build (and publish) the server communication port up front. */
    if (AcceptConnection)
    {
        Status = AlpcpCreateCommunicationPort(PreviousMode, ObjectAttributes, ConnectionPort, &ServerPort);
        if (!NT_SUCCESS(Status))
            goto Cleanup;

        ServerPort->PortContext = PortContext;

        Status = ObInsertObject(ServerPort, NULL, PORT_ALL_ACCESS, 0, NULL, &ServerHandle);
        if (!NT_SUCCESS(Status))
        {
            ServerPort = NULL; /* dereferenced by ObInsertObject */
            goto Cleanup;
        }
        /* Keep a reference while we finish the handshake. */
        ObReferenceObject(ServerPort);
    }

    KeAcquireGuardedMutex(&AlpcpLock);
    Message = ConnectionPort->CachedMessage;
    ClientPort = ConnectionPort->PendingClientPort;
    if (Message == NULL || ClientPort == NULL ||
        Message->PortMessage.MessageId != RequestMessageId)
    {
        KeReleaseGuardedMutex(&AlpcpLock);
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    WaitingThread = Message->WaitingThread;
    if (AcceptConnection)
    {
        ClientPort->u1.ConnectionPending = FALSE;

        /* Pair the communication ports so disconnecting one notifies the other. */
        if (ServerPort->CommunicationInfo != NULL && ClientPort->CommunicationInfo != NULL)
        {
            ServerPort->CommunicationInfo->ClientCommunicationPort = ClientPort;
            ClientPort->CommunicationInfo->ServerCommunicationPort = ServerPort;
        }
    }
    else
    {
        ClientPort->u1.ConnectionRefused = TRUE;
        ClientPort->u1.ConnectionPending = FALSE;
    }
    ConnectionPort->CachedMessage = NULL;
    ConnectionPort->PendingClientPort = NULL;
    KeReleaseGuardedMutex(&AlpcpLock);

    if (AcceptConnection)
    {
        *PortHandle = ServerHandle;
        ServerHandle = NULL; /* owned by the handle table now */
    }

    /* Wake the client last; it may free the connection message afterwards. */
    if (WaitingThread != NULL)
        KeReleaseSemaphore(&WaitingThread->AlpcWaitSemaphore, 0, 1, FALSE);

    Status = STATUS_SUCCESS;

Cleanup:
    if (ServerHandle != NULL)
        ObCloseHandle(ServerHandle, PreviousMode);
    if (ServerPort != NULL)
        ObDereferenceObject(ServerPort);
    if (ConnectionPort != NULL)
        ObDereferenceObject(ConnectionPort);

    return Status;
}

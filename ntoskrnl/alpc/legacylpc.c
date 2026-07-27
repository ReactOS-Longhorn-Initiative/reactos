/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Legacy LPC (NtCreatePort/NtConnectPort/...) backed by ALPC
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * On Windows the legacy LPC port API is a thin shim over ALPC: ports are ALPC
 * ports, and the legacy syscalls translate their classic parameters onto the
 * ALPC connection handshake and message queues. This file provides that shim for
 * ReactOS, reusing the ALPC primitives (AlpcpCreateConnectionPort,
 * AlpcpCreateCommunicationPort, AlpcpReceiveMessage, AlpcpSendRequest/Reply) so
 * the legacy surface and the native ALPC surface share one implementation. The
 * legacy port object type is the ALPC port object type.
 *
 * The two-step legacy accept (NtAcceptConnectPort then NtCompleteConnectPort)
 * keeps the connecting client blocked until the completion step, so the connection
 * request message is held on the accepted server port between the two calls.
 */

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

extern POBJECT_TYPE MmSectionObjectType;

/* GLOBALS ******************************************************************/

/* The legacy LPC port object types alias the ALPC port object type: legacy
 * ports are ALPC ports. Set up in LpcInitSystem. */
POBJECT_TYPE LpcPortObjectType;
POBJECT_TYPE LpcWaitablePortObjectType;
ULONG LpcpMaxMessageSize;

/* Connection-request type as produced/observed by the ALPC handshake. */
#define LPC_CONNECTION_REQUEST_TYPE (0x3000 | LPC_CONNECTION_REQUEST)

/* INITIALIZATION *********************************************************/

/**
 * @brief Brings up the legacy LPC layer (and the ALPC subsystem it rests on).
 *
 * Because legacy ports are ALPC ports, this initializes ALPC first, then points
 * the legacy port object types at the ALPC port object type. Called from Phase 1
 * of kernel init (before AlpcpInitSystem would otherwise run).
 *
 * @return TRUE on success.
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
LpcInitSystem(VOID)
{
    NTSTATUS Status;

    Status = AlpcpInitSystem();
    if (!NT_SUCCESS(Status))
        return FALSE;

    LpcPortObjectType = AlpcPortObjectType;
    LpcWaitablePortObjectType = AlpcPortObjectType;
    LpcpMaxMessageSize = PORT_MAXIMUM_MESSAGE_LENGTH;
    return TRUE;
}

/* HELPERS ***************************************************************/

/**
 * @brief Captures a legacy message (header + inline data) into a kernel buffer.
 *
 * The total length is normalized to DataLength + sizeof(PORT_MESSAGE), matching
 * the lenient clamping the LPC-mode send path performs.
 *
 * @return STATUS_SUCCESS with the output data buffer filled (caller frees it
 * with TAG_ALPC_MESSAGE), or an error.
 */
static
NTSTATUS
LpcpCaptureMessage(
    _In_ PPORT_MESSAGE UserMessage,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ ULONG MaxLength,
    _Out_ PPORT_MESSAGE OutHeader,
    _Outptr_result_maybenull_ PVOID *OutData,
    _Out_ PULONG OutDataLength)
{
    PORT_MESSAGE Header;
    ULONG DataLength;
    PVOID Data = NULL;

    *OutData = NULL;
    *OutDataLength = 0;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(UserMessage, sizeof(PORT_MESSAGE), sizeof(ULONG));
        Header = *UserMessage;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    DataLength = (USHORT)Header.u1.s1.DataLength;
    if (DataLength + sizeof(PORT_MESSAGE) > MaxLength)
        return STATUS_PORT_MESSAGE_TOO_LONG;

    /* Normalize the total length (LPC-mode clamping). */
    Header.u1.s1.TotalLength = (CSHORT)(DataLength + sizeof(PORT_MESSAGE));

    if (DataLength != 0)
    {
        Data = ExAllocatePoolWithTag(NonPagedPool, DataLength, TAG_ALPC_MESSAGE);
        if (Data == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForRead((PUCHAR)UserMessage + sizeof(PORT_MESSAGE), DataLength, 1);
            RtlCopyMemory(Data, (PUCHAR)UserMessage + sizeof(PORT_MESSAGE), DataLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ExFreePoolWithTag(Data, TAG_ALPC_MESSAGE);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    *OutHeader = Header;
    *OutData = Data;
    *OutDataLength = DataLength;
    return STATUS_SUCCESS;
}

/* PORT CREATION *********************************************************/

/**
 * @brief Creates a legacy LPC connection port (backed by an ALPC port).
 */
NTSTATUS
NTAPI
NtCreatePort(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectionInfoLength,
    _In_ ULONG MaxMessageLength,
    _In_ ULONG MaxPoolUsage)
{
    UNREFERENCED_PARAMETER(MaxConnectionInfoLength);
    UNREFERENCED_PARAMETER(MaxPoolUsage);

    /* Legacy MaxMessageLength is the maximum *total* message size (header +
     * data); it becomes the port's ALPC MaxMessageLength directly. */
    if (MaxMessageLength < sizeof(PORT_MESSAGE) || MaxMessageLength > 0xFFEF)
        return STATUS_INVALID_PARAMETER;

    return AlpcpCreateConnectionPort(PortHandle, ObjectAttributes, NULL,
                                     MaxMessageLength, FALSE, TRUE);
}

/**
 * @brief Creates a legacy waitable LPC connection port.
 */
NTSTATUS
NTAPI
NtCreateWaitablePort(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectInfoLength,
    _In_ ULONG MaxDataLength,
    _In_ ULONG NPMessageQueueSize)
{
    UNREFERENCED_PARAMETER(MaxConnectInfoLength);
    UNREFERENCED_PARAMETER(NPMessageQueueSize);

    if (MaxDataLength < sizeof(PORT_MESSAGE) || MaxDataLength > 0xFFEF)
        return STATUS_INVALID_PARAMETER;

    return AlpcpCreateConnectionPort(PortHandle, ObjectAttributes, NULL,
                                     MaxDataLength, TRUE, TRUE);
}

/* CONNECT **************************************************************/

/**
 * @brief Shared back-end for NtConnectPort / NtSecureConnectPort.
 *
 * Mirrors the ALPC connect handshake: open the named server port, create the
 * client communication port (ConnectionPending), queue a connection request and
 * block until the server accepts (NtCompleteConnectPort) or refuses.
 */
static
NTSTATUS
LpcpConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _Inout_opt_ PPORT_VIEW ClientView,
    _Inout_opt_ PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength,
    _Out_opt_ PULONG MaxMessageLength,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PETHREAD Thread = PsGetCurrentThread();
    UNICODE_STRING CapturedName;
    BOOLEAN NameCaptured = FALSE;
    PALPC_PORT ServerPort = NULL;
    PALPC_PORT ClientPort = NULL;
    PETHREAD Receiver;
    HANDLE ClientHandle = NULL;
    PKALPC_MESSAGE Message = NULL;
    ULONG ConnInfoLength = 0;
    PORT_VIEW CapturedClientView;
    BOOLEAN HaveClientView = FALSE;
    PVOID ViewSection = NULL;
    UCHAR ConnResponse[0x200];
    ULONG ConnResponseLength = 0;
    NTSTATUS Status;

    RtlZeroMemory(&CapturedClientView, sizeof(CapturedClientView));

    if (ConnectionInformation != NULL && ConnectionInformationLength != NULL)
    {
        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForRead(ConnectionInformationLength, sizeof(ULONG), sizeof(ULONG));
            ConnInfoLength = *ConnectionInformationLength;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    if (ClientView != NULL)
    {
        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForWrite(ClientView, sizeof(PORT_VIEW), sizeof(ULONG));
            CapturedClientView = *ClientView;
            HaveClientView = (CapturedClientView.Length == sizeof(PORT_VIEW) &&
                              CapturedClientView.SectionHandle != NULL);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    Status = ProbeAndCaptureUnicodeString(&CapturedName, PreviousMode, PortName);
    if (!NT_SUCCESS(Status))
        return Status;
    NameCaptured = TRUE;

    Status = ObReferenceObjectByName(&CapturedName, OBJ_CASE_INSENSITIVE, NULL, 0,
                                     AlpcPortObjectType, KernelMode, NULL,
                                     (PVOID *)&ServerPort);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    if (ConnInfoLength > ServerPort->PortAttributes.MaxMessageLength)
        ConnInfoLength = ServerPort->PortAttributes.MaxMessageLength;

    Status = AlpcpCreateCommunicationPort(PreviousMode, NULL, ServerPort, &ClientPort);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    ClientPort->u1.ConnectionPending = TRUE;

    /* Map the client's shared "write" section into this (client) process. The
     * server side is mapped at accept; the client polls both base addresses. */
    if (HaveClientView)
    {
        PVOID ViewBase = NULL;
        SIZE_T ViewSize = CapturedClientView.ViewSize;
        LARGE_INTEGER SectionOffset;

        Status = ObReferenceObjectByHandle(CapturedClientView.SectionHandle,
                                           SECTION_MAP_READ | SECTION_MAP_WRITE,
                                           MmSectionObjectType, PreviousMode,
                                           &ViewSection, NULL);
        if (!NT_SUCCESS(Status))
            goto Cleanup;

        SectionOffset.QuadPart = CapturedClientView.SectionOffset;
        Status = MmMapViewOfSection(ViewSection, PsGetCurrentProcess(), &ViewBase,
                                    0, 0, &SectionOffset, &ViewSize, ViewUnmap, 0,
                                    PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
            goto Cleanup;

        ClientPort->LpcViewSection = ViewSection;
        ClientPort->LpcClientViewBase = ViewBase;
        ClientPort->LpcViewSize = ViewSize;
        ViewSection = NULL; /* owned by the port now */

        CapturedClientView.ViewBase = ViewBase;
        CapturedClientView.ViewSize = ViewSize;
    }

    Status = ObInsertObject(ClientPort, NULL, PORT_ALL_ACCESS, 0, NULL, &ClientHandle);
    if (!NT_SUCCESS(Status))
    {
        ClientPort = NULL;
        goto Cleanup;
    }
    ObReferenceObject(ClientPort);

    Message = AlpcpAllocateMessage(ConnInfoLength);
    if (Message == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Message->PortMessage.u2.s2.Type = LPC_CONNECTION_REQUEST_TYPE;
    Message->PortMessage.ClientId = Thread->Cid;
    Message->OwnerPort = ClientPort;
    Message->ConnectionPort = ServerPort;
    Message->WaitingThread = Thread;

    if (ConnInfoLength != 0 && ConnectionInformation != NULL)
    {
        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForRead(ConnectionInformation, ConnInfoLength, 1);
            RtlCopyMemory(AlpcpGetMessageData(Message), ConnectionInformation, ConnInfoLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            goto Cleanup;
        }
        _SEH2_END;
    }

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

    /* Legacy connect is always synchronous: block until accept+complete. */
    Status = KeWaitForSingleObject(&Thread->AlpcWaitSemaphore, WrLpcReply,
                                   PreviousMode, FALSE, NULL);
    if (Status == STATUS_SUCCESS)
    {
        if (ClientPort->u1.ConnectionRefused)
            Status = STATUS_PORT_CONNECTION_REFUSED;
        else if (ClientPort->u1.ConnectionPending)
            Status = STATUS_PORT_CONNECTION_REFUSED;
        else
            Status = STATUS_SUCCESS;
    }

    /* Grab the server's connection-info response from the message before it is
     * reclaimed; it is handed back to the caller's ConnectionInformation. */
    if (Status == STATUS_SUCCESS && ConnInfoLength != 0)
    {
        ConnResponseLength = ConnInfoLength;
        if (ConnResponseLength > sizeof(ConnResponse))
            ConnResponseLength = sizeof(ConnResponse);
        RtlCopyMemory(ConnResponse, AlpcpGetMessageData(Message), ConnResponseLength);
    }

    /* Reclaim the connection request. */
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
        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForWriteHandle(PortHandle);
            *PortHandle = ClientHandle;

            if (MaxMessageLength != NULL)
                *MaxMessageLength = ClientPort->PortAttributes.MaxMessageLength;

            if (ClientView != NULL && HaveClientView)
            {
                CapturedClientView.ViewBase = ClientPort->LpcClientViewBase;
                CapturedClientView.ViewRemoteBase = ClientPort->LpcServerViewBase;
                *ClientView = CapturedClientView;
            }

            if (ConnectionInformation != NULL && ConnResponseLength != 0)
            {
                RtlCopyMemory(ConnectionInformation, ConnResponse, ConnResponseLength);
                if (ConnectionInformationLength != NULL)
                    *ConnectionInformationLength = ConnResponseLength;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (NT_SUCCESS(Status))
            ClientHandle = NULL;
    }

Cleanup:
    if (ViewSection != NULL)
        ObDereferenceObject(ViewSection);
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

NTSTATUS
NTAPI
NtConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Inout_opt_ PPORT_VIEW ClientView,
    _Inout_opt_ PREMOTE_PORT_VIEW ServerView,
    _Out_opt_ PULONG MaxMessageLength,
    _Inout_opt_ PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength)
{
    UNREFERENCED_PARAMETER(SecurityQos);
    UNREFERENCED_PARAMETER(ServerView);

    return LpcpConnectPort(PortHandle, PortName, ClientView, ConnectionInformation,
                           ConnectionInformationLength, MaxMessageLength,
                           KeGetPreviousMode());
}

NTSTATUS
NTAPI
NtSecureConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Inout_opt_ PPORT_VIEW ClientView,
    _In_opt_ PSID ServerSid,
    _Inout_opt_ PREMOTE_PORT_VIEW ServerView,
    _Out_opt_ PULONG MaxMessageLength,
    _Inout_opt_ PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength)
{
    UNREFERENCED_PARAMETER(SecurityQos);
    UNREFERENCED_PARAMETER(ServerSid);
    UNREFERENCED_PARAMETER(ServerView);

    return LpcpConnectPort(PortHandle, PortName, ClientView, ConnectionInformation,
                           ConnectionInformationLength, MaxMessageLength,
                           KeGetPreviousMode());
}

/* ACCEPT / COMPLETE ***************************************************/

/**
 * @brief Finds the connection port holding a cached connection request by id.
 *
 * @return Referenced connection port, or NULL.
 */
static
PALPC_PORT
LpcpFindConnectionPortByRequest(
    _In_ ULONG MessageId)
{
    PALPC_PORT Found = NULL;
    PLIST_ENTRY Entry;

    KeAcquireGuardedMutex(&AlpcpPortListLock);
    for (Entry = AlpcpPortList.Flink; Entry != &AlpcpPortList; Entry = Entry->Flink)
    {
        PALPC_PORT Port = CONTAINING_RECORD(Entry, ALPC_PORT, PortListEntry);
        if (Port->CachedMessage != NULL &&
            Port->CachedConnectionMessageId == MessageId)
        {
            Found = Port;
            ObReferenceObject(Found);
            break;
        }
    }
    KeReleaseGuardedMutex(&AlpcpPortListLock);

    return Found;
}

/**
 * @brief Accepts or refuses a legacy connection request.
 *
 * On acceptance a server communication port is created and returned, but the
 * connecting client stays blocked: the connection request message is parked on
 * the server port (CachedMessage) and the client is woken by
 * NtCompleteConnectPort. On refusal the client is woken immediately.
 */
NTSTATUS
NTAPI
NtAcceptConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_opt_ PVOID PortContext,
    _In_ PPORT_MESSAGE ConnectionRequest,
    _In_ BOOLEAN AcceptConnection,
    _Inout_opt_ PPORT_VIEW ServerView,
    _Out_opt_ PREMOTE_PORT_VIEW ClientView)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT ConnectionPort = NULL;
    PALPC_PORT ServerPort = NULL;
    PALPC_PORT ClientPort;
    PKALPC_MESSAGE Message;
    PETHREAD WaitingThread = NULL;
    HANDLE ServerHandle = NULL;
    ULONG RequestMessageId = 0;
    PALPC_PORT AcceptedClientPort = NULL;
    PVOID ViewSection = NULL;
    SIZE_T ViewSize = 0;
    UCHAR ResponseData[0x200];
    ULONG ResponseLength = 0;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ServerView);

    /* Capture the message id and the server's connection-information response
     * (the server fills it into the connection request before accepting); it is
     * handed back to the connecting client. */
    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
        {
            if (AcceptConnection)
                ProbeForWriteHandle(PortHandle);
            ProbeForRead(ConnectionRequest, sizeof(PORT_MESSAGE), sizeof(ULONG));
        }
        RequestMessageId = ConnectionRequest->MessageId;
        ResponseLength = (USHORT)ConnectionRequest->u1.s1.DataLength;
        if (ResponseLength > sizeof(ResponseData))
            ResponseLength = sizeof(ResponseData);
        if (ResponseLength != 0)
        {
            if (PreviousMode != KernelMode)
                ProbeForRead((PUCHAR)ConnectionRequest + sizeof(PORT_MESSAGE), ResponseLength, 1);
            RtlCopyMemory(ResponseData, (PUCHAR)ConnectionRequest + sizeof(PORT_MESSAGE), ResponseLength);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    ConnectionPort = LpcpFindConnectionPortByRequest(RequestMessageId);
    if (ConnectionPort == NULL)
        return STATUS_INVALID_PARAMETER;

    if (AcceptConnection)
    {
        Status = AlpcpCreateCommunicationPort(PreviousMode, NULL, ConnectionPort, &ServerPort);
        if (!NT_SUCCESS(Status))
            goto Cleanup;

        ServerPort->PortContext = PortContext;

        Status = ObInsertObject(ServerPort, NULL, PORT_ALL_ACCESS, 0, NULL, &ServerHandle);
        if (!NT_SUCCESS(Status))
        {
            ServerPort = NULL;
            goto Cleanup;
        }
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

    if (AcceptConnection)
    {
        /* Stash the server's connection-info response in the connection message
         * so the connecting client receives it back. */
        ULONG CopyLen = ResponseLength;
        if (CopyLen > (ULONG)(USHORT)Message->PortMessage.u1.s1.DataLength)
            CopyLen = (USHORT)Message->PortMessage.u1.s1.DataLength;
        if (CopyLen != 0)
            RtlCopyMemory(AlpcpGetMessageData(Message), ResponseData, CopyLen);

        ClientPort->u1.ConnectionPending = FALSE;
        if (ServerPort->CommunicationInfo != NULL && ClientPort->CommunicationInfo != NULL)
        {
            ServerPort->CommunicationInfo->ClientCommunicationPort = ClientPort;
            ClientPort->CommunicationInfo->ServerCommunicationPort = ServerPort;
        }
        /* Park the request on the server port; the client is woken by complete. */
        ServerPort->CachedMessage = Message;
        ServerPort->CachedConnectionMessageId = RequestMessageId;
        ConnectionPort->CachedMessage = NULL;
        ConnectionPort->PendingClientPort = NULL;
        /* Keep the client port alive while we map its section below. */
        AcceptedClientPort = ClientPort;
        ObReferenceObject(AcceptedClientPort);
        ViewSection = ClientPort->LpcViewSection;
        ViewSize = ClientPort->LpcViewSize;
        KeReleaseGuardedMutex(&AlpcpLock);

        /* Map the client's shared section into this (server) process so the
         * server can read client capture buffers; this base is reported back to
         * both the server (ClientView) and the client (LpcServerViewBase). */
        if (ViewSection != NULL)
        {
            PVOID ServerViewBase = NULL;
            SIZE_T MapSize = ViewSize;
            LARGE_INTEGER SectionOffset;

            SectionOffset.QuadPart = 0;
            Status = MmMapViewOfSection(ViewSection, PsGetCurrentProcess(), &ServerViewBase,
                                        0, 0, &SectionOffset, &MapSize, ViewUnmap, 0,
                                        PAGE_READWRITE);
            if (NT_SUCCESS(Status))
                AcceptedClientPort->LpcServerViewBase = ServerViewBase;

            if (NT_SUCCESS(Status) && ClientView != NULL)
            {
                _SEH2_TRY
                {
                    if (PreviousMode != KernelMode)
                        ProbeForWrite(ClientView, sizeof(REMOTE_PORT_VIEW), sizeof(ULONG));
                    ClientView->ViewBase = ServerViewBase;
                    ClientView->ViewSize = MapSize;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    NOTHING;
                }
                _SEH2_END;
            }
        }

        _SEH2_TRY
        {
            *PortHandle = ServerHandle;
            Status = STATUS_SUCCESS;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (NT_SUCCESS(Status))
            ServerHandle = NULL;
    }
    else
    {
        WaitingThread = Message->WaitingThread;
        ClientPort->u1.ConnectionRefused = TRUE;
        ClientPort->u1.ConnectionPending = FALSE;
        ConnectionPort->CachedMessage = NULL;
        ConnectionPort->PendingClientPort = NULL;
        KeReleaseGuardedMutex(&AlpcpLock);

        if (WaitingThread != NULL)
            KeReleaseSemaphore(&WaitingThread->AlpcWaitSemaphore, 0, 1, FALSE);
        Status = STATUS_SUCCESS;
    }

Cleanup:
    if (AcceptedClientPort != NULL)
        ObDereferenceObject(AcceptedClientPort);
    if (ServerHandle != NULL)
        ObCloseHandle(ServerHandle, PreviousMode);
    if (ServerPort != NULL)
        ObDereferenceObject(ServerPort);
    if (ConnectionPort != NULL)
        ObDereferenceObject(ConnectionPort);

    return Status;
}

/**
 * @brief Completes a legacy connection, waking the blocked client.
 */
NTSTATUS
NTAPI
NtCompleteConnectPort(
    _In_ HANDLE PortHandle)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT ServerPort;
    PKALPC_MESSAGE Message;
    PETHREAD WaitingThread = NULL;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&ServerPort, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    KeAcquireGuardedMutex(&AlpcpLock);
    Message = ServerPort->CachedMessage;
    if (Message != NULL)
    {
        WaitingThread = Message->WaitingThread;
        ServerPort->CachedMessage = NULL;
    }
    KeReleaseGuardedMutex(&AlpcpLock);

    if (WaitingThread != NULL)
        KeReleaseSemaphore(&WaitingThread->AlpcWaitSemaphore, 0, 1, FALSE);

    ObDereferenceObject(ServerPort);
    return STATUS_SUCCESS;
}

/* RECEIVE / REPLY ****************************************************/

/**
 * @brief Receives the next message on a port (optionally replying first).
 */
static
NTSTATUS
LpcpReplyWaitReceive(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PALPC_PORT Port;
    SIZE_T BufferLength;
    PVOID ServerContext = NULL;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Send the reply first, if any. */
    if (ReplyMessage != NULL)
    {
        PORT_MESSAGE ReplyHeader;
        PVOID ReplyData = NULL;
        ULONG ReplyDataLength = 0;

        Status = LpcpCaptureMessage(ReplyMessage, PreviousMode,
                                    Port->PortAttributes.MaxMessageLength,
                                    &ReplyHeader, &ReplyData, &ReplyDataLength);
        if (NT_SUCCESS(Status))
        {
            AlpcpSendReply(Port, &ReplyHeader, ReplyData, ReplyDataLength);
            if (ReplyData != NULL)
                ExFreePoolWithTag(ReplyData, TAG_ALPC_MESSAGE);
        }
    }

    BufferLength = Port->PortAttributes.MaxMessageLength;
    Status = AlpcpReceiveMessage(Port, ReceiveMessage, &BufferLength, NULL,
                                 &ServerContext, PreviousMode, Timeout);

    /* Return the per-client port context the server set at NtAcceptConnectPort. */
    if (NT_SUCCESS(Status) && PortContext != NULL)
    {
        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForWrite(PortContext, sizeof(PVOID), sizeof(PVOID));
            *PortContext = ServerContext;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            NOTHING;
        }
        _SEH2_END;
    }
    /* Deliver the plain LPC message type to legacy clients: strip the ALPC
     * internal high bits (0x3000) so a connection request reads as
     * LPC_CONNECTION_REQUEST (10), a request as LPC_REQUEST (1), etc. Legacy
     * servers (SMSS, CSRSS) compare the whole Type field, not just the low byte. */
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            ReceiveMessage->u2.s2.Type &= ~0x3000;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            NOTHING;
        }
        _SEH2_END;
    }

    ObDereferenceObject(Port);
    return Status;
}

NTSTATUS
NTAPI
NtReplyWaitReceivePort(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage)
{
    return LpcpReplyWaitReceive(PortHandle, PortContext, ReplyMessage,
                                ReceiveMessage, NULL, KeGetPreviousMode());
}

NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    LARGE_INTEGER CapturedTimeout;
    PLARGE_INTEGER TimeoutPtr = NULL;

    if (Timeout != NULL)
    {
        _SEH2_TRY
        {
            if (PreviousMode != KernelMode)
                ProbeForRead(Timeout, sizeof(LARGE_INTEGER), sizeof(ULONG));
            CapturedTimeout = *Timeout;
            TimeoutPtr = &CapturedTimeout;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    return LpcpReplyWaitReceive(PortHandle, PortContext, ReplyMessage,
                                ReceiveMessage, TimeoutPtr, PreviousMode);
}

/**
 * @brief Listens for the next connection request on a port.
 */
NTSTATUS
NTAPI
NtListenPort(
    _In_ HANDLE PortHandle,
    _Out_ PPORT_MESSAGE ConnectionRequest)
{
    return LpcpReplyWaitReceive(PortHandle, NULL, NULL, ConnectionRequest,
                                NULL, KeGetPreviousMode());
}

/**
 * @brief Replies to a message on a port (no receive).
 */
NTSTATUS
NTAPI
NtReplyPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE LpcReply)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    PORT_MESSAGE ReplyHeader;
    PVOID ReplyData = NULL;
    ULONG ReplyDataLength = 0;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = LpcpCaptureMessage(LpcReply, PreviousMode,
                                Port->PortAttributes.MaxMessageLength,
                                &ReplyHeader, &ReplyData, &ReplyDataLength);
    if (NT_SUCCESS(Status))
    {
        Status = AlpcpSendReply(Port, &ReplyHeader, ReplyData, ReplyDataLength);
        if (ReplyData != NULL)
            ExFreePoolWithTag(ReplyData, TAG_ALPC_MESSAGE);
    }

    ObDereferenceObject(Port);
    return Status;
}

NTSTATUS
NTAPI
NtReplyWaitReplyPort(
    _In_ HANDLE PortHandle,
    _Inout_ PPORT_MESSAGE ReplyMessage)
{
    /* Reply then wait for a reply: rarely used; treat as a plain reply. */
    return NtReplyPort(PortHandle, ReplyMessage);
}

/* REQUEST ************************************************************/

/**
 * @brief Shared send back-end for the legacy request syscalls and Lpc* helpers.
 */
static
NTSTATUS
LpcpRequest(
    _In_ PALPC_PORT Port,
    _In_ PPORT_MESSAGE RequestMessage,
    _Out_opt_ PPORT_MESSAGE ReplyMessage,
    _In_ BOOLEAN WaitForReply,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    PORT_MESSAGE Header;
    PVOID Data = NULL;
    ULONG DataLength = 0;
    SIZE_T ReplyBufferLength;
    NTSTATUS Status;

    Status = LpcpCaptureMessage(RequestMessage, PreviousMode,
                                Port->PortAttributes.MaxMessageLength,
                                &Header, &Data, &DataLength);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Legacy clients do not set MessageId (the kernel assigns it). Clear it so a
     * non-zero/high-bit value from the caller's buffer is not mistaken for a
     * resource-reserve handle by the ALPC send path. */
    Header.MessageId = 0;

    ReplyBufferLength = Port->PortAttributes.MaxMessageLength;
    Status = AlpcpSendRequest(Port, &Header, Data, DataLength,
                              WaitForReply ? ALPC_MSGFLG_SYNC_REQUEST : 0,
                              NULL, ReplyMessage,
                              ReplyMessage != NULL ? &ReplyBufferLength : NULL,
                              ReplyBufferLength, PreviousMode, NULL);

    if (Data != NULL)
        ExFreePoolWithTag(Data, TAG_ALPC_MESSAGE);
    return Status;
}

NTSTATUS
NTAPI
NtRequestPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE LpcMessage)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = LpcpRequest(Port, LpcMessage, NULL, FALSE, PreviousMode);

    ObDereferenceObject(Port);
    return Status;
}

NTSTATUS
NTAPI
NtRequestWaitReplyPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE LpcRequest,
    _Out_ PPORT_MESSAGE LpcReply)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(PortHandle, 0, AlpcPortObjectType, PreviousMode,
                                       (PVOID *)&Port, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = LpcpRequest(Port, LpcRequest, LpcReply, TRUE, PreviousMode);

    ObDereferenceObject(Port);
    return Status;
}

/* KERNEL-MODE SENDERS (dbgk / harderr / ps) *************************/

NTSTATUS
NTAPI
LpcRequestPort(
    _In_ PVOID Port,
    _In_ PPORT_MESSAGE LpcMessage)
{
    return LpcpRequest((PALPC_PORT)Port, LpcMessage, NULL, FALSE, KernelMode);
}

NTSTATUS
NTAPI
LpcRequestWaitReplyPort(
    _In_ PVOID Port,
    _In_ PPORT_MESSAGE LpcMessageRequest,
    _Out_ PPORT_MESSAGE LpcMessageReply)
{
    return LpcpRequest((PALPC_PORT)Port, LpcMessageRequest, LpcMessageReply,
                       TRUE, KernelMode);
}

/* IMPERSONATION / QUERY / MISC ************************************/

NTSTATUS
NTAPI
NtImpersonateClientOfPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE ClientMessage)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PALPC_PORT Port;
    ULONG MessageId;
    NTSTATUS Status;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(ClientMessage, sizeof(PORT_MESSAGE), sizeof(ULONG));
        MessageId = ClientMessage->MessageId;
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

    Status = AlpcpImpersonateMessage(Port, MessageId);

    ObDereferenceObject(Port);
    return Status;
}

NTSTATUS
NTAPI
NtQueryInformationPort(
    _In_ HANDLE PortHandle,
    _In_ PORT_INFORMATION_CLASS PortInformationClass,
    _Out_ PVOID PortInformation,
    _In_ ULONG PortInformationLength,
    _Out_ PULONG ReturnLength)
{
    UNREFERENCED_PARAMETER(PortHandle);
    UNREFERENCED_PARAMETER(PortInformationClass);
    UNREFERENCED_PARAMETER(PortInformation);
    UNREFERENCED_PARAMETER(PortInformationLength);
    UNREFERENCED_PARAMETER(ReturnLength);
    return STATUS_INVALID_INFO_CLASS;
}

NTSTATUS
NTAPI
NtQueryPortInformationProcess(VOID)
{
    /* Returns whether the calling process owns any LPC ports; we report none. */
    return STATUS_NOT_FOUND;
}

NTSTATUS
NTAPI
NtReadRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG Index,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength)
{
    UNREFERENCED_PARAMETER(PortHandle);
    UNREFERENCED_PARAMETER(Message);
    UNREFERENCED_PARAMETER(Index);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferLength);
    UNREFERENCED_PARAMETER(ReturnLength);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtWriteRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG Index,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength)
{
    UNREFERENCED_PARAMETER(PortHandle);
    UNREFERENCED_PARAMETER(Message);
    UNREFERENCED_PARAMETER(Index);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferLength);
    UNREFERENCED_PARAMETER(ReturnLength);
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief Per-thread LPC cleanup at thread exit.
 *
 * The ALPC layer keys synchronous waits on the thread's own semaphore and holds
 * no per-thread reply chain, so there is nothing to unwind here.
 */
VOID
NTAPI
LpcExitThread(
    _In_ PETHREAD Thread)
{
    UNREFERENCED_PARAMETER(Thread);
}

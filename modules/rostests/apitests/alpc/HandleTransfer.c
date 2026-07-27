/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Cross-process handle passing via the ALPC HANDLE message attribute
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * The client attaches an event handle as an ALPC_HANDLE_ATTR. The kernel
 * duplicates the underlying object into the receiver, which gets its own handle
 * to the same event - proven here by the server signalling it and the client
 * observing its original event become signalled.
 */

#include "precomp.h"

#include <process.h>

typedef struct _HT_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} HT_MSG, *PHT_MSG;

typedef struct _HT_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
    BOOLEAN GotHandle;
} HT_CTX, *PHT_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PHT_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { HT_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    HT_MSG Reply;
    UCHAR RecvAttrBuffer[256];
    PALPC_MESSAGE_ATTRIBUTES RecvAttr = (PALPC_MESSAGE_ATTRIBUTES)RecvAttrBuffer;
    PALPC_HANDLE_ATTR RecvHandle;
    SIZE_T BufferLength, Required;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    ok_hex(Ctx->AcceptStatus, STATUS_SUCCESS);
    if (!NT_SUCCESS(Ctx->AcceptStatus))
        return 0;

    Status = AlpcInitializeMessageAttribute(ALPC_MESSAGE_HANDLE_ATTRIBUTE, RecvAttr,
                                            sizeof(RecvAttrBuffer), &Required);
    ok_hex(Status, STATUS_SUCCESS);

    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, RecvAttr, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);

    if (NT_SUCCESS(Status))
    {
        ok(RecvAttr->ValidAttributes & ALPC_MESSAGE_HANDLE_ATTRIBUTE,
           "handle attribute not delivered (ValidAttributes=0x%lx)\n", RecvAttr->ValidAttributes);
        RecvHandle = AlpcGetMessageAttribute(RecvAttr, ALPC_MESSAGE_HANDLE_ATTRIBUTE);
        ok(RecvHandle != NULL, "received handle attribute is NULL\n");
        if (RecvHandle && (RecvAttr->ValidAttributes & ALPC_MESSAGE_HANDLE_ATTRIBUTE))
        {
            ok(RecvHandle->Handle != NULL, "received handle is NULL\n");
            if (RecvHandle->Handle != NULL)
            {
                /* The duplicated handle must be a usable event in this process. */
                Status = NtSetEvent(RecvHandle->Handle, NULL);
                ok_hex(Status, STATUS_SUCCESS);
                if (NT_SUCCESS(Status))
                    Ctx->GotHandle = TRUE;
                NtClose(RecvHandle->Handle);
            }
        }
    }

    /* Reply so the client's synchronous send returns. */
    RtlZeroMemory(&Reply, sizeof(Reply));
    Reply.Header = Recv.Raw.Header;
    Reply.Header.u1.s1.DataLength = sizeof(ULONG);
    Reply.Header.u1.s1.TotalLength = sizeof(HT_MSG);
    Reply.Value = 0;
    NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, ALPC_MSGFLG_REPLY_MESSAGE,
                              &Reply.Header, NULL, NULL, NULL, NULL, &Timeout);
    return 0;
}

START_TEST(HandleTransfer)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    HANDLE Event = NULL;
    HT_CTX Ctx;
    HT_MSG Request;
    union { HT_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Reply;
    UCHAR SendAttrBuffer[256];
    PALPC_MESSAGE_ATTRIBUTES SendAttr = (PALPC_MESSAGE_ATTRIBUTES)SendAttrBuffer;
    PALPC_HANDLE_ATTR SendHandle;
    SIZE_T BufferLength, Required;
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    Status = NtCreateEvent(&Event, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return;

    AlpcMakeUniquePortName(&PortName, NameBuffer, RTL_NUMBER_OF(NameBuffer));
    Status = AlpcCreateServerPort(&ServerPort, &PortName, ALPC_TEST_PORT_MAXMSG);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create server port\n");
        NtClose(Event);
        return;
    }

    RtlZeroMemory(&Ctx, sizeof(Ctx));
    Ctx.ConnectionPort = ServerPort;
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        Status = AlpcInitializeMessageAttribute(ALPC_MESSAGE_HANDLE_ATTRIBUTE, SendAttr,
                                                sizeof(SendAttrBuffer), &Required);
        ok_hex(Status, STATUS_SUCCESS);
        SendHandle = AlpcGetMessageAttribute(SendAttr, ALPC_MESSAGE_HANDLE_ATTRIBUTE);
        ok(SendHandle != NULL, "send handle attribute is NULL\n");
        if (SendHandle)
        {
            SendHandle->Flags = 0;
            SendHandle->Handle = Event;
            SendHandle->ObjectType = 0;
            SendHandle->DesiredAccess = EVENT_MODIFY_STATE | SYNCHRONIZE;
            SendAttr->ValidAttributes |= ALPC_MESSAGE_HANDLE_ATTRIBUTE;

            RtlZeroMemory(&Request, sizeof(Request));
            AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
            Request.Value = 0;
            RtlZeroMemory(&Reply, sizeof(Reply));
            BufferLength = sizeof(Reply);
            Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                               &Request.Header, SendAttr,
                                               &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
            ok_hex(Status, STATUS_SUCCESS);

            /* The server signalled the duplicated handle, so our original event
             * (the same object) must now be signalled. */
            if (NT_SUCCESS(Status))
            {
                LARGE_INTEGER Zero;
                Zero.QuadPart = 0;
                Status = NtWaitForSingleObject(Event, FALSE, &Zero);
                ok(Status == STATUS_WAIT_0,
                   "event not signalled after handle round-trip (0x%lx)\n", Status);
            }
        }
    }

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    ok(Ctx.GotHandle, "server did not receive a usable handle\n");

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
    NtClose(Event);
}

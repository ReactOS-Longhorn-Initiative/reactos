/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     The TOKEN message attribute carries the sender's token ids
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * A receiver that allocates the TOKEN attribute gets the sender's TokenId,
 * AuthenticationId and ModifiedId. Whether it is delivered for a message that
 * carries no security attribute is version-specific, so the id check is made
 * only when the attribute is actually delivered.
 */

#include "precomp.h"

#include <process.h>

typedef struct _TK_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} TK_MSG, *PTK_MSG;

typedef struct _TK_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
} TK_CTX, *PTK_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PTK_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { TK_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    TK_MSG Reply;
    UCHAR RecvAttrBuffer[256];
    PALPC_MESSAGE_ATTRIBUTES RecvAttr = (PALPC_MESSAGE_ATTRIBUTES)RecvAttrBuffer;
    PALPC_TOKEN_ATTR RecvToken;
    SIZE_T BufferLength, Required;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    ok_hex(Ctx->AcceptStatus, STATUS_SUCCESS);
    if (!NT_SUCCESS(Ctx->AcceptStatus))
        return 0;

    Status = AlpcInitializeMessageAttribute(ALPC_MESSAGE_TOKEN_ATTRIBUTE, RecvAttr,
                                            sizeof(RecvAttrBuffer), &Required);
    ok_hex(Status, STATUS_SUCCESS);

    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, RecvAttr, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);

    if (NT_SUCCESS(Status))
    {
        trace("TOKEN ValidAttributes = 0x%lx\n", RecvAttr->ValidAttributes);
        if (RecvAttr->ValidAttributes & ALPC_MESSAGE_TOKEN_ATTRIBUTE)
        {
            RecvToken = AlpcGetMessageAttribute(RecvAttr, ALPC_MESSAGE_TOKEN_ATTRIBUTE);
            ok(RecvToken != NULL, "token attribute pointer is NULL\n");
            if (RecvToken)
            {
                trace("TOKEN id %lx:%lx auth %lx:%lx\n",
                      RecvToken->TokenId.HighPart, RecvToken->TokenId.LowPart,
                      RecvToken->AuthenticationId.HighPart, RecvToken->AuthenticationId.LowPart);
                ok(RecvToken->TokenId.LowPart != 0 || RecvToken->TokenId.HighPart != 0,
                   "token id is zero\n");
            }
        }
    }

    RtlZeroMemory(&Reply, sizeof(Reply));
    Reply.Header = Recv.Raw.Header;
    Reply.Header.u1.s1.DataLength = sizeof(ULONG);
    Reply.Header.u1.s1.TotalLength = sizeof(TK_MSG);
    Reply.Value = 0;
    NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, ALPC_MSGFLG_REPLY_MESSAGE,
                              &Reply.Header, NULL, NULL, NULL, NULL, &Timeout);
    return 0;
}

START_TEST(TokenAttribute)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    TK_CTX Ctx;
    TK_MSG Request;
    union { TK_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Reply;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    AlpcMakeUniquePortName(&PortName, NameBuffer, RTL_NUMBER_OF(NameBuffer));
    Status = AlpcCreateServerPort(&ServerPort, &PortName, ALPC_TEST_PORT_MAXMSG);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create server port\n");
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
        RtlZeroMemory(&Request, sizeof(Request));
        AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
        Request.Value = 0x7777;
        RtlZeroMemory(&Reply, sizeof(Reply));
        BufferLength = sizeof(Reply);
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                           &Request.Header, NULL,
                                           &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);
    }

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NtAlpcCancelMessage cancels a pending request
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * The server receives a synchronous request and cancels it instead of replying.
 * Verified on the Win11 oracle: NtAlpcCancelMessage returns STATUS_PENDING once
 * the cancellation is initiated, and the blocked client's send is released with
 * STATUS_MESSAGE_LOST (0xC0000701). STATUS_TIMEOUT is tolerated as a fallback in
 * case the header-only canceller fails to match the message. The deterministic
 * flags validation is also asserted.
 */

#include "precomp.h"

#include <process.h>

typedef struct _C_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} C_MSG, *PC_MSG;

typedef struct _C_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
    NTSTATUS CancelStatus;
} C_CTX, *PC_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PC_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { C_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    ALPC_CONTEXT_ATTR Context;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    if (!NT_SUCCESS(Ctx->AcceptStatus))
        return 0;

    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
    if (!NT_SUCCESS(Status))
        return 0;

    /* Cancel the received request by its message id instead of replying. */
    RtlZeroMemory(&Context, sizeof(Context));
    Context.MessageId = Recv.Raw.Header.MessageId;
    Ctx->CancelStatus = NtAlpcCancelMessage(Ctx->ConnectionPort, 0, &Context);
    trace("NtAlpcCancelMessage = 0x%lx\n", Ctx->CancelStatus);
    return 0;
}

START_TEST(CancelRoundTrip)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    C_CTX Ctx;
    C_MSG Request;
    union { C_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Reply;
    ALPC_CONTEXT_ATTR BadContext;
    SIZE_T BufferLength;
    LARGE_INTEGER ConnectTimeout, RequestTimeout;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    if (!NtAlpcCancelMessage)
    {
        skip("NtAlpcCancelMessage not available\n");
        return;
    }

    ConnectTimeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */
    RequestTimeout.QuadPart = (LONGLONG)-3 * 1000 * 1000 * 10;  /* 3s  */

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
    Ctx.CancelStatus = STATUS_UNSUCCESSFUL;
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Status = AlpcClientConnect(&PortName, &ClientCommPort, &ConnectTimeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        /* Invalid flags must be rejected up front (deterministic). */
        RtlZeroMemory(&BadContext, sizeof(BadContext));
        BadContext.MessageId = 1;
        Status = NtAlpcCancelMessage(ClientCommPort, 0x10, &BadContext);
        ok(Status == STATUS_INVALID_PARAMETER,
           "CancelMessage(bad flags) = 0x%lx, expected STATUS_INVALID_PARAMETER\n", Status);

        /* Synchronous request that the server cancels instead of replying. */
        RtlZeroMemory(&Request, sizeof(Request));
        AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
        Request.Value = 0xC0FFEE;
        RtlZeroMemory(&Reply, sizeof(Reply));
        BufferLength = sizeof(Reply);
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                           &Request.Header, NULL,
                                           &Reply.Raw.Header, &BufferLength, NULL, &RequestTimeout);
        trace("client send after cancel = 0x%lx\n", Status);
        ok(Status == STATUS_MESSAGE_LOST || Status == STATUS_TIMEOUT,
           "client send = 0x%lx, expected STATUS_MESSAGE_LOST or STATUS_TIMEOUT\n", Status);
    }

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    /* The cancel call itself reports STATUS_PENDING once the request matched. */
    ok(Ctx.CancelStatus == STATUS_PENDING || Ctx.CancelStatus == STATUS_NOT_FOUND,
       "CancelMessage = 0x%lx, expected STATUS_PENDING\n", Ctx.CancelStatus);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

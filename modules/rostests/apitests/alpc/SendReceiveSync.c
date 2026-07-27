/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Synchronous request/reply round-trip over NtAlpcSendWaitReceivePort
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

#include <process.h>

#define REQUEST_MAGIC 0x11112222
#define REPLY_MAGIC   0x3333aaaa

typedef struct _TEST_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} TEST_MSG, *PTEST_MSG;

typedef struct _SRV_CTX
{
    HANDLE ConnectionPort;
    NTSTATUS Status;
} SRV_CTX, *PSRV_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PSRV_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    HANDLE CommPort = NULL;
    union { TEST_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    TEST_MSG Reply;
    SIZE_T BufferLength;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Ctx->Status = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                      &CommPort, &ConnRequest);
    ok_hex(Ctx->Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Ctx->Status))
        return 0;

    /*
     * The server receives client data messages on the CONNECTION port (the
     * comm port from accept is only an identifier for impersonation/queries).
     */
    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        ok(ALPC_MSG_TYPE(Recv.Raw.Header) == LPC_REQUEST,
           "request Type = %x (base %x), expected LPC_REQUEST\n",
           Recv.Raw.Header.u2.s2.Type, ALPC_MSG_TYPE(Recv.Raw.Header));
        ok(Recv.Raw.Header.u1.s1.DataLength == sizeof(ULONG),
           "request DataLength = %u\n", Recv.Raw.Header.u1.s1.DataLength);
        ok(Recv.Msg.Value == REQUEST_MAGIC,
           "request Value = 0x%lx, expected 0x%x\n", Recv.Msg.Value, REQUEST_MAGIC);
        ok(Recv.Raw.Header.MessageId != 0, "request MessageId is 0\n");

        /* Build and send the reply on the connection port, echoing identity. */
        RtlZeroMemory(&Reply, sizeof(Reply));
        Reply.Header = Recv.Raw.Header;
        Reply.Header.u1.s1.DataLength = sizeof(ULONG);
        Reply.Header.u1.s1.TotalLength = sizeof(TEST_MSG);
        Reply.Value = REPLY_MAGIC;

        Status = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, ALPC_MSGFLG_REPLY_MESSAGE,
                                           &Reply.Header, NULL, NULL, NULL, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);
    }

    NtClose(CommPort);
    return 0;
}

START_TEST(SendReceiveSync)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    SRV_CTX Ctx;
    TEST_MSG Request;
    union { TEST_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Reply;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;
    ULONG RequestId;

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
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to connect\n");
        goto Cleanup;
    }

    /* Send the request and wait for the reply in a single call. */
    RtlZeroMemory(&Request, sizeof(Request));
    AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
    Request.Value = REQUEST_MAGIC;

    RtlZeroMemory(&Reply, sizeof(Reply));
    BufferLength = sizeof(Reply);
    Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                       &Request.Header, NULL,
                                       &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        ok(Reply.Msg.Value == REPLY_MAGIC,
           "reply Value = 0x%lx, expected 0x%x\n", Reply.Msg.Value, REPLY_MAGIC);
        /*
         * Native ALPC does NOT reclassify the reply's PORT_MESSAGE.Type to
         * LPC_REPLY; replies carry LPC_REQUEST in the type low byte and are
         * correlated to the request by MessageId. The real reply signal is the
         * round-tripped payload (REPLY_MAGIC) returned to this sync call.
         * Confirmed against the Win11 oracle: reply Type == 0x3001, base 0x01.
         */
        ok(ALPC_MSG_TYPE(Reply.Raw.Header) == LPC_REQUEST,
           "reply Type = %x (base %x), expected LPC_REQUEST\n",
           Reply.Raw.Header.u2.s2.Type, ALPC_MSG_TYPE(Reply.Raw.Header));
        RequestId = Reply.Raw.Header.MessageId;
        ok(RequestId != 0, "reply MessageId is 0\n");
    }

Cleanup:
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    if (ClientCommPort)
        NtClose(ClientCommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

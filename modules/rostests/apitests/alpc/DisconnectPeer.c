/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Disconnecting a communication port does not affect its peer
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * A communication port is only an identifier for the connection; disconnecting
 * it does not tear down the underlying connection. Verified on the Win11 oracle:
 * after the server disconnects its communication port, the client's send (routed
 * through the still-live connection port) succeeds. The disconnect is NOT
 * propagated to the peer.
 */

#include "precomp.h"

#include <process.h>

typedef struct _DP_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} DP_MSG, *PDP_MSG;

typedef struct _DP_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
    NTSTATUS DisconnectStatus;
} DP_CTX, *PDP_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PDP_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    if (!NT_SUCCESS(Ctx->AcceptStatus))
        return 0;

    /* Disconnect the server side of the communication. */
    Ctx->DisconnectStatus = NtAlpcDisconnectPort(Ctx->CommPort, 0);
    ok_hex(Ctx->DisconnectStatus, STATUS_SUCCESS);
    return 0;
}

START_TEST(DisconnectPeer)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    DP_CTX Ctx;
    DP_MSG Request;
    LARGE_INTEGER ConnectTimeout, SendTimeout;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    ConnectTimeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */
    SendTimeout.QuadPart = (LONGLONG)-2 * 1000 * 1000 * 10;     /* 2s  */

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

    Status = AlpcClientConnect(&PortName, &ClientCommPort, &ConnectTimeout);
    ok_hex(Status, STATUS_SUCCESS);

    /* Wait for the server to accept and then disconnect its comm port. */
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    ok_hex(Ctx.AcceptStatus, STATUS_SUCCESS);

    if (NT_SUCCESS(Status) && NT_SUCCESS(Ctx.AcceptStatus))
    {
        /*
         * The peer (client) port is unaffected: its send through the live
         * connection port still succeeds, since disconnecting a comm port does
         * not tear down the connection.
         */
        RtlZeroMemory(&Request, sizeof(Request));
        AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
        Request.Value = 1;
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, &Request.Header, NULL,
                                           NULL, NULL, NULL, &SendTimeout);
        ok(Status == STATUS_SUCCESS,
           "send on peer after disconnect = 0x%lx, expected STATUS_SUCCESS\n", Status);
    }

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

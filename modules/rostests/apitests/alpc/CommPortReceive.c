/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Client traffic is received on the connection port, not the comm port
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * The per-client communication port returned by NtAlpcAcceptConnectPort is only
 * an identifier (for impersonation/queries). The server receives client data on
 * the CONNECTION port, so a receive on the comm port simply blocks and times out.
 */

#include "precomp.h"

#include <process.h>

typedef struct _COMM_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
    NTSTATUS ReceiveStatus;
} COMM_CTX, *PCOMM_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PCOMM_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    ALPC_TEST_MESSAGE_BUFFER Recv;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    if (!NT_SUCCESS(Ctx->AcceptStatus))
        return 0;

    /* Receiving on the comm port must time out: no traffic is queued there. */
    Timeout.QuadPart = (LONGLONG)-1 * 1000 * 1000 * 10; /* 1s */
    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Ctx->ReceiveStatus = NtAlpcSendWaitReceivePort(Ctx->CommPort, 0, NULL, NULL,
                                                   &Recv.Header, &BufferLength, NULL, &Timeout);
    return 0;
}

START_TEST(CommPortReceive)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    COMM_CTX Ctx;
    LARGE_INTEGER Timeout;

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

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */
    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    ok_hex(Ctx.AcceptStatus, STATUS_SUCCESS);
    ok(Ctx.ReceiveStatus == STATUS_TIMEOUT,
       "receive on comm port = 0x%lx, expected STATUS_TIMEOUT\n", Ctx.ReceiveStatus);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

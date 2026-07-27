/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcDisconnectPort
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

#include <process.h>

typedef struct _SRV_CTX
{
    HANDLE ConnectionPort;
    HANDLE StopEvent;
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

    Ctx->Status = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                      &CommPort, &ConnRequest);
    ok_hex(Ctx->Status, STATUS_SUCCESS);

    /* Hold the connection open until the client is done testing disconnect. */
    NtWaitForSingleObject(Ctx->StopEvent, FALSE, NULL);

    if (CommPort)
        NtClose(CommPort);
    return 0;
}

START_TEST(NtAlpcDisconnectPort)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    SRV_CTX Ctx;
    struct { PORT_MESSAGE Header; ULONG Value; } Msg;
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
    Status = NtCreateEvent(&Ctx.StopEvent, EVENT_ALL_ACCESS, NULL,
                           NotificationEvent, FALSE);
    ok_hex(Status, STATUS_SUCCESS);

    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to connect\n");
        NtSetEvent(Ctx.StopEvent, NULL);
        goto Cleanup;
    }

    /* Disconnect the client side of the communication. */
    Status = NtAlpcDisconnectPort(ClientCommPort, 0);
    ok_hex(Status, STATUS_SUCCESS);

    /* Any further send on the disconnected port must be rejected. */
    RtlZeroMemory(&Msg, sizeof(Msg));
    AlpcInitMessageHeader(&Msg.Header, sizeof(ULONG));
    Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, &Msg.Header, NULL,
                                       NULL, NULL, NULL, &Timeout);
    ok(Status == STATUS_PORT_DISCONNECTED,
       "send after disconnect = 0x%lx, expected STATUS_PORT_DISCONNECTED\n", Status);

    NtSetEvent(Ctx.StopEvent, NULL);

Cleanup:
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    if (ClientCommPort)
        NtClose(ClientCommPort);
    NtClose(ThreadHandle);
    NtClose(Ctx.StopEvent);
    NtClose(ServerPort);
}

/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcConnectPort (client side of the handshake)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

#include <process.h>

typedef struct _SERVER_CONTEXT
{
    HANDLE ConnectionPort;
    BOOLEAN Accept;
    NTSTATUS AcceptStatus;
    HANDLE CommPort;
} SERVER_CONTEXT, *PSERVER_CONTEXT;

static
UINT
CALLBACK
AcceptServerThread(
    _Inout_ PVOID Parameter)
{
    PSERVER_CONTEXT Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort,
                                            Ctx->Accept,
                                            NULL,
                                            &Ctx->CommPort,
                                            &ConnRequest);
    return 0;
}

static
VOID
Test_NonexistentPort(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE CommPort;

    /* A name we never create must fail cleanly. */
    AlpcMakeUniquePortName(&PortName, NameBuffer, RTL_NUMBER_OF(NameBuffer));
    Status = AlpcClientConnect(&PortName, &CommPort, NULL);
    ok_hex(Status, STATUS_OBJECT_NAME_NOT_FOUND);
}

static
VOID
Test_Handshake(
    _In_ BOOLEAN Accept)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    SERVER_CONTEXT Ctx;
    LARGE_INTEGER Timeout;

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
    Ctx.Accept = Accept;

    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, AcceptServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    /* Bound the client wait so a broken implementation can't hang the suite. */
    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */
    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);

    if (Accept)
    {
        ok_hex(Status, STATUS_SUCCESS);
        ok(ClientCommPort != NULL, "ClientCommPort is NULL after accept\n");
    }
    else
    {
        ok_hex(Status, STATUS_PORT_CONNECTION_REFUSED);
    }

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    if (Accept)
    {
        ok_hex(Ctx.AcceptStatus, STATUS_SUCCESS);
        ok(Ctx.CommPort != NULL, "Server CommPort is NULL after accept\n");
    }

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

START_TEST(NtAlpcConnectPort)
{
    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    Test_NonexistentPort();
    Test_Handshake(TRUE);
    Test_Handshake(FALSE);
}

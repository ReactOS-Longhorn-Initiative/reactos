/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcConnectPortEx (connect by object attributes)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NtAlpcConnectPortEx is the same handshake as NtAlpcConnectPort but identifies
 * the connection port through an OBJECT_ATTRIBUTES (ObjectName) instead of a
 * bare UNICODE_STRING name.
 */

#include "precomp.h"

#include <process.h>

typedef struct _SERVER_CTX
{
    HANDLE ConnectionPort;
    NTSTATUS Status;
} SERVER_CTX, *PSERVER_CTX;

static
UINT
CALLBACK
AcceptServerThread(
    _Inout_ PVOID Parameter)
{
    PSERVER_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    HANDLE CommPort = NULL;

    Ctx->Status = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL, &CommPort, &ConnRequest);
    if (CommPort)
        NtClose(CommPort);
    return 0;
}

START_TEST(NtAlpcConnectPortEx)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    SERVER_CTX Ctx;
    OBJECT_ATTRIBUTES ConnPortAttributes;
    ALPC_PORT_ATTRIBUTES PortAttributes;
    ALPC_TEST_MESSAGE_BUFFER ConnMsg;
    SIZE_T ConnMsgLen;
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }
    if (!NtAlpcConnectPortEx)
    {
        skip("NtAlpcConnectPortEx not available\n");
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
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, AcceptServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    /* Connect via OBJECT_ATTRIBUTES referencing the named connection port. */
    InitializeObjectAttributes(&ConnPortAttributes, &PortName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    AlpcInitDefaultPortAttributes(&PortAttributes, ALPC_TEST_PORT_MAXMSG);
    RtlZeroMemory(&ConnMsg, sizeof(ConnMsg));
    AlpcInitMessageHeader(&ConnMsg.Header, 0);
    ConnMsgLen = sizeof(ConnMsg);

    Status = NtAlpcConnectPortEx(&ClientCommPort,
                                 &ConnPortAttributes,
                                 NULL,
                                 &PortAttributes,
                                 ALPC_MSGFLG_SYNC_REQUEST,
                                 NULL,
                                 &ConnMsg.Header,
                                 &ConnMsgLen,
                                 NULL,
                                 NULL,
                                 &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    ok(ClientCommPort != NULL || !NT_SUCCESS(Status), "ClientCommPort is NULL after success\n");

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    ok_hex(Ctx.Status, STATUS_SUCCESS);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

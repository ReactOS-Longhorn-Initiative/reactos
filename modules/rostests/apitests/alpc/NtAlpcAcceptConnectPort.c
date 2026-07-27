/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcAcceptConnectPort (server side of the handshake)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

#include <process.h>

static UCHAR MagicContext;

typedef struct _ACCEPT_CONTEXT
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS Status;
} ACCEPT_CONTEXT, *PACCEPT_CONTEXT;

static
UINT
CALLBACK
AcceptServerThread(
    _Inout_ PVOID Parameter)
{
    PACCEPT_CONTEXT Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    ALPC_BASIC_INFORMATION BasicInfo;
    ULONG ReturnLength;
    NTSTATUS Status;

    /* Accept, attaching a known port context. */
    Ctx->Status = AlpcServerAcceptOne(Ctx->ConnectionPort,
                                      TRUE,
                                      &MagicContext,
                                      &Ctx->CommPort,
                                      &ConnRequest);
    ok_hex(Ctx->Status, STATUS_SUCCESS);

    /* The connection request must be typed as a connection request. */
    ok(ALPC_MSG_TYPE(ConnRequest.Header) == LPC_CONNECTION_REQUEST,
       "connection request Type = %x (base %x)\n",
       ConnRequest.Header.u2.s2.Type, ALPC_MSG_TYPE(ConnRequest.Header));
    ok(ConnRequest.Header.ClientId.UniqueProcess == UlongToHandle(GetCurrentProcessId()),
       "connection request from foreign process %p\n",
       ConnRequest.Header.ClientId.UniqueProcess);

    if (!NT_SUCCESS(Ctx->Status))
        return 0;

    /* The port context we passed must be observable on the comm port. */
    RtlZeroMemory(&BasicInfo, sizeof(BasicInfo));
    Status = NtAlpcQueryInformation(Ctx->CommPort, AlpcBasicInformation,
                                    &BasicInfo, sizeof(BasicInfo), &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok(BasicInfo.PortContext == &MagicContext,
       "PortContext = %p, expected %p\n", BasicInfo.PortContext, &MagicContext);

    return 0;
}

START_TEST(NtAlpcAcceptConnectPort)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    ACCEPT_CONTEXT Ctx;
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
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, AcceptServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */
    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    ok(ClientCommPort != NULL, "ClientCommPort is NULL\n");

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

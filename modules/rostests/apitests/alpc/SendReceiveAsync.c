/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Datagram (no-reply) send and FIFO receive ordering
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

#include <process.h>

#define MSG_COUNT 4

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
    SIZE_T BufferLength;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;
    ULONG i;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Ctx->Status = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                      &CommPort, &ConnRequest);
    ok_hex(Ctx->Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Ctx->Status))
        return 0;

    /* Datagrams must be delivered in send order, received on the connection port. */
    for (i = 0; i < MSG_COUNT; i++)
    {
        RtlZeroMemory(&Recv, sizeof(Recv));
        BufferLength = sizeof(Recv);
        Status = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, 0, NULL, NULL,
                                           &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);
        if (Status != STATUS_SUCCESS)
            break;
        /*
         * Native ALPC types every normal data message as LPC_REQUEST in the
         * PORT_MESSAGE.Type low byte (there is no distinct received "datagram"
         * type). Confirmed against the Win11 oracle: Type == 0x3001, base 0x01.
         */
        ok(ALPC_MSG_TYPE(Recv.Raw.Header) == LPC_REQUEST,
           "msg %lu Type = %x (base %x), expected LPC_REQUEST\n",
           i, Recv.Raw.Header.u2.s2.Type, ALPC_MSG_TYPE(Recv.Raw.Header));
        ok(Recv.Msg.Value == i,
           "msg %lu Value = %lu, expected %lu (out of order?)\n", i, Recv.Msg.Value, i);
    }

    NtClose(CommPort);
    return 0;
}

START_TEST(SendReceiveAsync)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    SRV_CTX Ctx;
    TEST_MSG Msg;
    LARGE_INTEGER Timeout;
    ULONG i;

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

    /* Fire several datagrams without waiting for any reply. */
    for (i = 0; i < MSG_COUNT; i++)
    {
        RtlZeroMemory(&Msg, sizeof(Msg));
        AlpcInitMessageHeader(&Msg.Header, sizeof(ULONG));
        Msg.Value = i;
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, &Msg.Header, NULL,
                                           NULL, NULL, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);
    }

Cleanup:
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    if (ClientCommPort)
        NtClose(ClientCommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

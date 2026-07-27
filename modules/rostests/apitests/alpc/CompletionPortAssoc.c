/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Associating an ALPC port with an I/O completion port
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NtAlpcSetInformation(AlpcAssociateCompletionPortInformation) binds an ALPC
 * connection port to an I/O completion port. The information length is validated
 * (verified against the Win11 oracle: the 8-byte struct succeeds, a short buffer
 * returns STATUS_INFO_LENGTH_MISMATCH), a port may be associated only once, and
 * a message arriving with no thread blocked in receive posts a completion packet
 * carrying the association's completion key.
 */

#include "precomp.h"

#include <process.h>

typedef struct _CPA_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} CPA_MSG, *PCPA_MSG;

typedef struct _CPA_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
} CPA_CTX, *PCPA_CTX;

/* Server thread: only accept the connection, then exit (no data receive), so a
 * subsequent client message has no waiter and posts a completion packet. */
static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PCPA_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    ok_hex(Ctx->AcceptStatus, STATUS_SUCCESS);
    return 0;
}

START_TEST(CompletionPortAssoc)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    HANDLE IoCompletion = NULL;
    CPA_CTX Ctx;
    CPA_MSG Request;
    ALPC_PORT_ASSOCIATE_COMPLETION_PORT Associate;
    PVOID CompletionKey = (PVOID)(ULONG_PTR)0xA1B2C3D4;
    LARGE_INTEGER Timeout, RemoveTimeout;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10;     /* 10s */
    RemoveTimeout.QuadPart = (LONGLONG)-5 * 1000 * 1000 * 10; /* 5s  */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    if (!NtAlpcSetInformation)
    {
        skip("NtAlpcSetInformation not available\n");
        return;
    }

    Status = NtCreateIoCompletion(&IoCompletion, IO_COMPLETION_ALL_ACCESS, NULL, 0);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create I/O completion port\n");
        return;
    }

    AlpcMakeUniquePortName(&PortName, NameBuffer, RTL_NUMBER_OF(NameBuffer));
    Status = AlpcCreateServerPort(&ServerPort, &PortName, ALPC_TEST_PORT_MAXMSG);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create server port\n");
        NtClose(IoCompletion);
        return;
    }

    /* A short information buffer must be rejected. */
    Associate.CompletionKey = CompletionKey;
    Associate.CompletionPort = IoCompletion;
    Status = NtAlpcSetInformation(ServerPort, AlpcAssociateCompletionPortInformation,
                                  &Associate, sizeof(Associate) - 1);
    ok(Status == STATUS_INFO_LENGTH_MISMATCH,
       "associate(short) = 0x%lx, expected STATUS_INFO_LENGTH_MISMATCH\n", Status);

    /* A correctly sized association succeeds. */
    Status = NtAlpcSetInformation(ServerPort, AlpcAssociateCompletionPortInformation,
                                  &Associate, sizeof(Associate));
    ok_hex(Status, STATUS_SUCCESS);

    /* A second association on the same port is refused. */
    Status = NtAlpcSetInformation(ServerPort, AlpcAssociateCompletionPortInformation,
                                  &Associate, sizeof(Associate));
    ok(Status == STATUS_PORT_ALREADY_SET,
       "associate(again) = 0x%lx, expected STATUS_PORT_ALREADY_SET\n", Status);

    RtlZeroMemory(&Ctx, sizeof(Ctx));
    Ctx.ConnectionPort = ServerPort;
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);

    /* Wait for the accept so no thread is blocked in receive on the port. */
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    if (NT_SUCCESS(Status) && NT_SUCCESS(Ctx.AcceptStatus))
    {
        PVOID OutKey = NULL;
        PVOID OutContext = NULL;
        IO_STATUS_BLOCK IoStatusBlock;

        /* Datagram send: no receiver waiting -> completion packet posted. */
        RtlZeroMemory(&Request, sizeof(Request));
        AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
        Request.Value = 0xC0A1;
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, &Request.Header, NULL,
                                           NULL, NULL, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);

        RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));
        Status = NtRemoveIoCompletion(IoCompletion, &OutKey, &OutContext,
                                      &IoStatusBlock, &RemoveTimeout);
        ok_hex(Status, STATUS_SUCCESS);
        ok(OutKey == CompletionKey,
           "completion key %p != expected %p\n", OutKey, CompletionKey);
    }

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
    NtClose(IoCompletion);
}

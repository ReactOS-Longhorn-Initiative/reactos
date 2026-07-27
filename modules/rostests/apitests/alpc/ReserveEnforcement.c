/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     A resource reserve backs exactly one in-flight send
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NtAlpcCreateResourceReserve returns a high-bit-tagged handle. A send whose
 * PORT_MESSAGE.MessageId carries that handle consumes the reserve: the first
 * such send succeeds, a second with the same reserve fails with
 * STATUS_RESOURCE_IN_USE, and an unknown reserve fails with
 * STATUS_OBJECTID_NOT_FOUND. Creation validation (Flags must be zero, the size
 * is bounded) and these codes were verified against the Win11 oracle.
 */

#include "precomp.h"

#include <process.h>

#define ALPC_RESERVE_HANDLE_TAG 0x80000000

typedef struct _RE_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} RE_MSG, *PRE_MSG;

typedef struct _RE_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
} RE_CTX, *PRE_CTX;

/* Server: accept, then service a single request with a reply. */
static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PRE_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { RE_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    RE_MSG Reply;
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

    RtlZeroMemory(&Reply, sizeof(Reply));
    Reply.Header = Recv.Raw.Header;
    Reply.Header.u1.s1.DataLength = sizeof(ULONG);
    Reply.Header.u1.s1.TotalLength = sizeof(RE_MSG);
    NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, ALPC_MSGFLG_REPLY_MESSAGE,
                              &Reply.Header, NULL, NULL, NULL, NULL, &Timeout);
    return 0;
}

START_TEST(ReserveEnforcement)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    RE_CTX Ctx;
    RE_MSG Request;
    union { RE_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Reply;
    ALPC_HANDLE ReserveId = NULL;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    if (!NtAlpcCreateResourceReserve)
    {
        skip("NtAlpcCreateResourceReserve not available\n");
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
    if (NT_SUCCESS(Status))
    {
        /* Creation validation. */
        Status = NtAlpcCreateResourceReserve(ClientCommPort, 1, 256, &ReserveId);
        ok(Status == STATUS_INVALID_PARAMETER,
           "reserve(flags=1) = 0x%lx, expected STATUS_INVALID_PARAMETER\n", Status);

        ReserveId = NULL;
        Status = NtAlpcCreateResourceReserve(ClientCommPort, 0, 0xFFD8, &ReserveId);
        ok(!NT_SUCCESS(Status),
           "reserve(oversize) = 0x%lx, expected failure\n", Status);

        /* A valid reserve returns a high-bit-tagged handle. */
        ReserveId = NULL;
        Status = NtAlpcCreateResourceReserve(ClientCommPort, 0, 256, &ReserveId);
        ok_hex(Status, STATUS_SUCCESS);
        ok(((ULONG_PTR)ReserveId & ALPC_RESERVE_HANDLE_TAG) != 0,
           "reserve handle %p is not tagged\n", ReserveId);

        if (NT_SUCCESS(Status))
        {
            /* First send consumes the reserve. */
            RtlZeroMemory(&Request, sizeof(Request));
            AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
            Request.Header.MessageId = (ULONG)(ULONG_PTR)ReserveId;
            Request.Value = 0x12345;
            RtlZeroMemory(&Reply, sizeof(Reply));
            BufferLength = sizeof(Reply);
            Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                               &Request.Header, NULL,
                                               &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
            ok_hex(Status, STATUS_SUCCESS);

            /* Second send with the same reserve is refused (still active). */
            RtlZeroMemory(&Request, sizeof(Request));
            AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
            Request.Header.MessageId = (ULONG)(ULONG_PTR)ReserveId;
            RtlZeroMemory(&Reply, sizeof(Reply));
            BufferLength = sizeof(Reply);
            Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                               &Request.Header, NULL,
                                               &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
            ok(Status == STATUS_RESOURCE_IN_USE,
               "send#2 same reserve = 0x%lx, expected STATUS_RESOURCE_IN_USE\n", Status);

            /* An unknown reserve handle fails to resolve. */
            RtlZeroMemory(&Request, sizeof(Request));
            AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
            Request.Header.MessageId = ALPC_RESERVE_HANDLE_TAG | 0x999;
            RtlZeroMemory(&Reply, sizeof(Reply));
            BufferLength = sizeof(Reply);
            Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                               &Request.Header, NULL,
                                               &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
            ok(Status == STATUS_OBJECTID_NOT_FOUND,
               "send#3 bogus reserve = 0x%lx, expected STATUS_OBJECTID_NOT_FOUND\n", Status);

            if (NtAlpcDeleteResourceReserve)
                NtAlpcDeleteResourceReserve(ClientCommPort, 0, ReserveId);
        }
    }

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcQueryInformationMessage (AlpcMessageSidInformation)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * Like the sender-open calls, AlpcpQuerySidMessage looks the message up in the
 * port queues, so the sender must be blocked on a synchronous request. The
 * server queries the message's SID and confirms it is the caller's user SID.
 */

#include "precomp.h"

#include <process.h>

#define MSG_MAGIC   0x51d00001
#define REPLY_MAGIC 0x51d000ff

typedef struct _CLIENT_CTX
{
    PUNICODE_STRING PortName;
} CLIENT_CTX, *PCLIENT_CTX;

typedef struct _TEST_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} TEST_MSG, *PTEST_MSG;

static
UINT
CALLBACK
ClientThread(
    _Inout_ PVOID Parameter)
{
    PCLIENT_CTX Ctx = Parameter;
    HANDLE ClientCommPort = NULL;
    TEST_MSG Request;
    union { TEST_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Reply;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Status = AlpcClientConnect(Ctx->PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return 0;

    RtlZeroMemory(&Request, sizeof(Request));
    AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
    Request.Value = MSG_MAGIC;

    RtlZeroMemory(&Reply, sizeof(Reply));
    BufferLength = sizeof(Reply);
    Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                       &Request.Header, NULL,
                                       &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);

    NtClose(ClientCommPort);
    return 0;
}

START_TEST(NtAlpcQueryInformationMessage)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE CommPort = NULL;
    HANDLE ThreadHandle;
    CLIENT_CTX Ctx;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { TEST_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    TEST_MSG ReplyMsg;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;
    UCHAR MessageSid[256];
    UCHAR ExpectedSid[256];
    ULONG ReturnLength;

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
    Ctx.PortName = &PortName;
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ClientThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Status = AlpcServerAcceptOne(ServerPort, TRUE, NULL, &CommPort, &ConnRequest);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to accept\n");
        goto Cleanup;
    }

    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(ServerPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status != STATUS_SUCCESS)
    {
        skip("Failed to receive request\n");
        goto Cleanup;
    }

    if (!NtAlpcQueryInformationMessage)
    {
        skip("NtAlpcQueryInformationMessage not available\n");
        goto Reply;
    }

    /* Query the sender's SID from the message. */
    RtlZeroMemory(MessageSid, sizeof(MessageSid));
    ReturnLength = 0;
    Status = NtAlpcQueryInformationMessage(ServerPort, &Recv.Raw.Header,
                                           AlpcMessageSidInformation,
                                           MessageSid, sizeof(MessageSid), &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        ok(ReturnLength != 0 && ReturnLength <= sizeof(MessageSid),
           "ReturnLength = %lu\n", ReturnLength);
        ok(RtlValidSid((PSID)MessageSid), "queried message SID is invalid\n");

        /* It must equal the current process's user SID (same user). */
        Status = AlpcGetCurrentUserSid(ExpectedSid, sizeof(ExpectedSid));
        ok_hex(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
        {
            ok(RtlEqualSid((PSID)MessageSid, (PSID)ExpectedSid),
               "message SID does not match the current user SID\n");
        }
    }

Reply:
    RtlZeroMemory(&ReplyMsg, sizeof(ReplyMsg));
    ReplyMsg.Header = Recv.Raw.Header;
    ReplyMsg.Header.u1.s1.DataLength = sizeof(ULONG);
    ReplyMsg.Header.u1.s1.TotalLength = sizeof(TEST_MSG);
    ReplyMsg.Value = REPLY_MAGIC;
    NtAlpcSendWaitReceivePort(ServerPort, ALPC_MSGFLG_REPLY_MESSAGE,
                              &ReplyMsg.Header, NULL, NULL, NULL, NULL, &Timeout);

Cleanup:
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    if (CommPort)
        NtClose(CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

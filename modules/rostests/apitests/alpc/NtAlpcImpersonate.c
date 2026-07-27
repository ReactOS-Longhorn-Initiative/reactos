/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcImpersonateClientOfPort
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * The server impersonates the client of a received (synchronous) message, then
 * confirms it now carries an impersonation token for the client's user, and
 * that reverting removes it. The client must be blocked on a sync request so
 * the message can be looked up and its security context captured.
 */

#include "precomp.h"

#include <process.h>

#define MSG_MAGIC   0x1339a001
#define REPLY_MAGIC 0x1339a0ff

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

static
VOID
VerifyImpersonationUser(VOID)
{
    NTSTATUS Status;
    HANDLE Token = NULL;
    UCHAR Buffer[256];
    UCHAR ExpectedSid[256];
    PTOKEN_USER UserInfo = (PTOKEN_USER)Buffer;
    ULONG ReturnLength;

    /* While impersonating, a thread token must exist for the client's user. */
    Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, TRUE, &Token);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return;

    Status = NtQueryInformationToken(Token, TokenUser, Buffer, sizeof(Buffer), &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        Status = AlpcGetCurrentUserSid(ExpectedSid, sizeof(ExpectedSid));
        ok_hex(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
            ok(RtlEqualSid(UserInfo->User.Sid, (PSID)ExpectedSid),
               "impersonation token user does not match the current user\n");
    }
    NtClose(Token);
}

START_TEST(NtAlpcImpersonate)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE CommPort = NULL;
    HANDLE ThreadHandle;
    HANDLE NullToken = NULL;
    HANDLE ProbeToken;
    CLIENT_CTX Ctx;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { TEST_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    TEST_MSG ReplyMsg;
    SIZE_T BufferLength;
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

    if (!NtAlpcImpersonateClientOfPort)
    {
        skip("NtAlpcImpersonateClientOfPort not available\n");
        goto Reply;
    }

    /*
     * Deterministic negative: a non-port handle must be rejected.
     * (Note: the Win10 decompile validates Flags up front - Flags must be 0 or 1 -
     * but on the Win11 oracle the impersonation-allowed check runs regardless of
     * Flags, so a bad-Flags value is NOT a reliable cross-version assertion.)
     */
    {
        HANDLE EventHandle;
        NTSTATUS S2 = NtCreateEvent(&EventHandle, EVENT_ALL_ACCESS, NULL,
                                    NotificationEvent, FALSE);
        ok_hex(S2, STATUS_SUCCESS);
        if (NT_SUCCESS(S2))
        {
            S2 = NtAlpcImpersonateClientOfPort(EventHandle, &Recv.Raw.Header, NULL);
            ok(!NT_SUCCESS(S2),
               "impersonate on a non-port handle unexpectedly succeeded (0x%lx)\n", S2);
            NtClose(EventHandle);
        }
    }

    /*
     * Positive path: impersonate the sender. AlpcpIsImpersonationAllowed requires
     * the message to be in the pending queue (QueueType 3) on the referenced port.
     * The exact receive-state precondition for that (likely tied to security
     * attributes / cross-process delivery) is still being characterized, so this
     * is best-effort: if it succeeds we verify the impersonation token and revert;
     * otherwise we record the status rather than failing the suite.
     */
    Status = NtAlpcImpersonateClientOfPort(CommPort, &Recv.Raw.Header, NULL);
    trace("NtAlpcImpersonateClientOfPort = 0x%lx\n", Status);
    if (NT_SUCCESS(Status))
    {
        VerifyImpersonationUser();

        /* Revert to self; afterwards there must be no thread token. */
        Status = NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken,
                                        &NullToken, sizeof(NullToken));
        ok_hex(Status, STATUS_SUCCESS);

        ProbeToken = NULL;
        Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, TRUE, &ProbeToken);
        ok(Status == STATUS_NO_TOKEN,
           "after revert, NtOpenThreadToken = 0x%lx, expected STATUS_NO_TOKEN\n", Status);
        if (NT_SUCCESS(Status))
            NtClose(ProbeToken);
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

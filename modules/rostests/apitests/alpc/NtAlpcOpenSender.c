/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcOpenSenderProcess / NtAlpcOpenSenderThread
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NtAlpcOpenSenderThread opens the message's WaitingThread, so it only works
 * for a SYNCHRONOUS request where the sender is blocked waiting for the reply.
 * The client therefore issues a synchronous request and the server opens the
 * sender (process and thread) before replying. NtAlpcOpenSenderProcess works
 * from the message's owner process and would also succeed for a datagram.
 */

#include "precomp.h"

#include <process.h>

#define SENDER_MAGIC 0x5e4de401
#define REPLY_MAGIC  0x5e4de4ff

typedef struct _CLIENT_CTX
{
    PUNICODE_STRING PortName;
    ULONG ClientTid;
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

    Ctx->ClientTid = GetCurrentThreadId();
    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Status = AlpcClientConnect(Ctx->PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return 0;

    /*
     * Synchronous request: this thread stays alive and registered as the
     * message's waiting thread until the server replies, which is what lets
     * NtAlpcOpenSenderThread open it.
     */
    RtlZeroMemory(&Request, sizeof(Request));
    AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
    Request.Value = SENDER_MAGIC;

    RtlZeroMemory(&Reply, sizeof(Reply));
    BufferLength = sizeof(Reply);
    Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                       &Request.Header, NULL,
                                       &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);

    NtClose(ClientCommPort);
    return 0;
}

START_TEST(NtAlpcOpenSender)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE CommPort = NULL;
    HANDLE ThreadHandle;
    HANDLE ProcessHandle, SenderThreadHandle;
    CLIENT_CTX Ctx;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { TEST_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    TEST_MSG ReplyMsg;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;
    PROCESS_BASIC_INFORMATION ProcessInfo;
    THREAD_BASIC_INFORMATION ThreadInfo;

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

    /* Accept the connection, then receive the client's synchronous request. */
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
        skip("Failed to receive sender message\n");
        goto Cleanup;
    }

    /* The message must identify the sending process and thread. */
    ok(Recv.Raw.Header.ClientId.UniqueProcess == UlongToHandle(GetCurrentProcessId()),
       "sender process = %p, expected %lu\n",
       Recv.Raw.Header.ClientId.UniqueProcess, GetCurrentProcessId());
    ok(Recv.Raw.Header.ClientId.UniqueThread == UlongToHandle(Ctx.ClientTid),
       "sender thread = %p, expected %lu\n",
       Recv.Raw.Header.ClientId.UniqueThread, Ctx.ClientTid);

    if (!NtAlpcOpenSenderProcess || !NtAlpcOpenSenderThread)
    {
        skip("NtAlpcOpenSender* not available (ALPC not yet implemented)\n");
        goto Reply;
    }

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    /* Open the sender process and confirm its PID. */
    ProcessHandle = NULL;
    Status = NtAlpcOpenSenderProcess(&ProcessHandle, ServerPort, &Recv.Raw.Header, 0,
                                     PROCESS_QUERY_INFORMATION, &ObjectAttributes);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
        Status = NtQueryInformationProcess(ProcessHandle, ProcessBasicInformation,
                                           &ProcessInfo, sizeof(ProcessInfo), NULL);
        ok_hex(Status, STATUS_SUCCESS);
        ok((ULONG_PTR)ProcessInfo.UniqueProcessId == (ULONG_PTR)GetCurrentProcessId(),
           "opened PID = %Iu, expected %lu\n",
           (ULONG_PTR)ProcessInfo.UniqueProcessId, GetCurrentProcessId());
        NtClose(ProcessHandle);
    }

    /* Open the sender thread (the waiting thread) and confirm its TID. */
    SenderThreadHandle = NULL;
    Status = NtAlpcOpenSenderThread(&SenderThreadHandle, ServerPort, &Recv.Raw.Header, 0,
                                    THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION,
                                    &ObjectAttributes);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(&ThreadInfo, sizeof(ThreadInfo));
        Status = NtQueryInformationThread(SenderThreadHandle, ThreadBasicInformation,
                                          &ThreadInfo, sizeof(ThreadInfo), NULL);
        ok_hex(Status, STATUS_SUCCESS);
        ok(ThreadInfo.ClientId.UniqueThread == UlongToHandle(Ctx.ClientTid),
           "opened TID = %p, expected %lu\n",
           ThreadInfo.ClientId.UniqueThread, Ctx.ClientTid);
        NtClose(SenderThreadHandle);
    }

Reply:
    /* Reply to release the client from its synchronous request. */
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

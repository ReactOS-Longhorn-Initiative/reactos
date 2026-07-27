/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Sender identification for a datagram (no waiting thread)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NtAlpcOpenSenderProcess and the message SID resolve the sender from the
 * message's recorded owner, so they work for a datagram - which, unlike a
 * synchronous request, has no waiting thread. NtAlpcOpenSenderThread therefore
 * cannot open a thread for a datagram and fails.
 */

#include "precomp.h"

#include <process.h>

#define DGRAM_MAGIC 0x5A5A1234

typedef struct _DG_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} DG_MSG, *PDG_MSG;

typedef struct _DG_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
} DG_CTX, *PDG_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PDG_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { DG_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PROCESS_BASIC_INFORMATION ProcessInfo;
    UCHAR MessageSid[256];
    ULONG ReturnLength;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    NTSTATUS Status;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    ok_hex(Ctx->AcceptStatus, STATUS_SUCCESS);
    if (!NT_SUCCESS(Ctx->AcceptStatus))
        return 0;

    /* Receive the datagram on the connection port. */
    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return 0;

    ok(ALPC_MSG_TYPE(Recv.Raw.Header) == LPC_REQUEST,
       "datagram Type = %x\n", Recv.Raw.Header.u2.s2.Type);
    ok(Recv.Msg.Value == DGRAM_MAGIC, "datagram Value = 0x%lx\n", Recv.Msg.Value);
    ok(Recv.Raw.Header.ClientId.UniqueProcess == UlongToHandle(GetCurrentProcessId()),
       "datagram from foreign process %p\n", Recv.Raw.Header.ClientId.UniqueProcess);

    if (!NtAlpcOpenSenderProcess || !NtAlpcOpenSenderThread || !NtAlpcQueryInformationMessage)
    {
        skip("ALPC sender-query API not available\n");
        return 0;
    }

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    /* OpenSenderProcess resolves the owner process even for a datagram. */
    ProcessHandle = NULL;
    Status = NtAlpcOpenSenderProcess(&ProcessHandle, Ctx->ConnectionPort, &Recv.Raw.Header, 0,
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

    /* A datagram has no waiting thread, so OpenSenderThread must fail (Win11
     * returns STATUS_ACCESS_DENIED while the sender is alive). */
    ThreadHandle = NULL;
    Status = NtAlpcOpenSenderThread(&ThreadHandle, Ctx->ConnectionPort, &Recv.Raw.Header, 0,
                                    THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION,
                                    &ObjectAttributes);
    ok(!NT_SUCCESS(Status),
       "OpenSenderThread on a datagram unexpectedly succeeded (0x%lx)\n", Status);
    trace("OpenSenderThread(datagram) = 0x%lx\n", Status);
    if (NT_SUCCESS(Status))
        NtClose(ThreadHandle);

    /* The message SID is also resolvable for a datagram. */
    RtlZeroMemory(MessageSid, sizeof(MessageSid));
    ReturnLength = 0;
    Status = NtAlpcQueryInformationMessage(Ctx->ConnectionPort, &Recv.Raw.Header,
                                           AlpcMessageSidInformation,
                                           MessageSid, sizeof(MessageSid), &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
        ok(RtlValidSid((PSID)MessageSid), "queried datagram SID is invalid\n");

    return 0;
}

START_TEST(OpenSenderDatagram)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    DG_CTX Ctx;
    DG_MSG Datagram;
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
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */
    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        /* Send a datagram (no SYNC_REQUEST: no waiting thread). */
        RtlZeroMemory(&Datagram, sizeof(Datagram));
        AlpcInitMessageHeader(&Datagram.Header, sizeof(ULONG));
        Datagram.Value = DGRAM_MAGIC;
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, &Datagram.Header, NULL,
                                           NULL, NULL, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);
    }

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

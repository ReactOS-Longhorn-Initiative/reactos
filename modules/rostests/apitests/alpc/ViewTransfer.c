/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     End-to-end shared-memory transfer via an ALPC VIEW message attribute
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * The client maps a section view, writes a pattern, and sends a message carrying
 * an ALPC_DATA_VIEW_ATTR (ALPC_MESSAGE_VIEW_ATTRIBUTE). The server receives with a
 * view-attribute buffer; the kernel exposes the mapped view and the server reads
 * the pattern back. This drives the full message-attribute path, not just the
 * section/view lifecycle.
 */

#include "precomp.h"

#include <process.h>

#define VIEW_SIZE    0x2000
#define VIEW_PATTERN 0x3C

typedef struct _CLIENT_CTX
{
    PUNICODE_STRING PortName;
    NTSTATUS Status;
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
    ALPC_HANDLE Section = NULL;
    SIZE_T ActualSize = 0;
    ALPC_DATA_VIEW_ATTR View;
    TEST_MSG Request;
    union { TEST_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Reply;
    UCHAR SendAttrBuffer[256];
    PALPC_MESSAGE_ATTRIBUTES SendAttr = (PALPC_MESSAGE_ATTRIBUTES)SendAttrBuffer;
    PALPC_DATA_VIEW_ATTR SendView;
    SIZE_T Required, BufferLength;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    RtlZeroMemory(&View, sizeof(View));

    Status = AlpcClientConnect(Ctx->PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return 0;

    /* Section + view, then write the pattern the server should read back. */
    Status = NtAlpcCreatePortSection(ClientCommPort, 0, NULL, VIEW_SIZE, &Section, &ActualSize);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    View.SectionHandle = Section;
    View.ViewSize = ActualSize;
    Status = NtAlpcCreateSectionView(ClientCommPort, 0, &View);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    RtlFillMemory(View.ViewBase, VIEW_SIZE, VIEW_PATTERN);

    /* Build the send attributes carrying the view. */
    Required = 0;
    Status = AlpcInitializeMessageAttribute(ALPC_MESSAGE_VIEW_ATTRIBUTE, SendAttr,
                                            sizeof(SendAttrBuffer), &Required);
    ok_hex(Status, STATUS_SUCCESS);
    SendView = AlpcGetMessageAttribute(SendAttr, ALPC_MESSAGE_VIEW_ATTRIBUTE);
    ok(SendView != NULL, "send view attribute is NULL\n");
    if (SendView)
    {
        SendView->Flags = 0;
        SendView->SectionHandle = Section;
        SendView->ViewBase = View.ViewBase;
        SendView->ViewSize = View.ViewSize;
        SendAttr->ValidAttributes |= ALPC_MESSAGE_VIEW_ATTRIBUTE;
    }

    RtlZeroMemory(&Request, sizeof(Request));
    AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
    Request.Value = VIEW_PATTERN;

    RtlZeroMemory(&Reply, sizeof(Reply));
    BufferLength = sizeof(Reply);
    Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                       &Request.Header, SendAttr,
                                       &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    Ctx->Status = Status;

Cleanup:
    if (View.ViewBase != NULL)
        NtAlpcDeleteSectionView(ClientCommPort, 0, View.ViewBase);
    if (Section != NULL)
        NtAlpcDeletePortSection(ClientCommPort, 0, Section);
    NtClose(ClientCommPort);
    return 0;
}

START_TEST(ViewTransfer)
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
    UCHAR RecvAttrBuffer[256];
    PALPC_MESSAGE_ATTRIBUTES RecvAttr = (PALPC_MESSAGE_ATTRIBUTES)RecvAttrBuffer;
    PALPC_DATA_VIEW_ATTR RecvView;
    SIZE_T Required, BufferLength;
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }
    if (!NtAlpcCreatePortSection || !NtAlpcCreateSectionView)
    {
        skip("ALPC section/view API not available\n");
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

    /* Receive with a view-attribute buffer so the kernel exposes the view. */
    Required = 0;
    Status = AlpcInitializeMessageAttribute(ALPC_MESSAGE_VIEW_ATTRIBUTE, RecvAttr,
                                            sizeof(RecvAttrBuffer), &Required);
    ok_hex(Status, STATUS_SUCCESS);

    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(ServerPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, RecvAttr, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status != STATUS_SUCCESS)
    {
        skip("Failed to receive request\n");
        goto Reply;
    }

    /* The view attribute must be present and carry the client's pattern. */
    ok(RecvAttr->ValidAttributes & ALPC_MESSAGE_VIEW_ATTRIBUTE,
       "view attribute not delivered (ValidAttributes=0x%lx)\n", RecvAttr->ValidAttributes);
    RecvView = AlpcGetMessageAttribute(RecvAttr, ALPC_MESSAGE_VIEW_ATTRIBUTE);
    ok(RecvView != NULL, "received view attribute is NULL\n");
    if (RecvView && (RecvAttr->ValidAttributes & ALPC_MESSAGE_VIEW_ATTRIBUTE))
    {
        PUCHAR Bytes = RecvView->ViewBase;
        ok(RecvView->ViewBase != NULL, "received ViewBase is NULL\n");
        ok(RecvView->ViewSize >= VIEW_SIZE, "received ViewSize = %Iu\n", RecvView->ViewSize);
        if (Bytes)
            ok(Bytes[0] == VIEW_PATTERN && Bytes[VIEW_SIZE - 1] == VIEW_PATTERN,
               "view data mismatch: [0]=%02x [end]=%02x, expected %02x\n",
               Bytes[0], Bytes[VIEW_SIZE - 1], VIEW_PATTERN);
    }

Reply:
    RtlZeroMemory(&ReplyMsg, sizeof(ReplyMsg));
    ReplyMsg.Header = Recv.Raw.Header;
    ReplyMsg.Header.u1.s1.DataLength = sizeof(ULONG);
    ReplyMsg.Header.u1.s1.TotalLength = sizeof(TEST_MSG);
    ReplyMsg.Value = VIEW_PATTERN;
    NtAlpcSendWaitReceivePort(ServerPort, ALPC_MSGFLG_REPLY_MESSAGE,
                              &ReplyMsg.Header, NULL, NULL, NULL, NULL, &Timeout);

Cleanup:
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    if (CommPort)
        NtClose(CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

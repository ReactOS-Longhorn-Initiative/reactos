/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Legacy LPC (NtCreatePort/NtConnectPort/...) round-trip over ALPC
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * Exercises the legacy LPC port API, which on ReactOS is backed by the ALPC
 * layer (ntoskrnl/alpc/legacylpc.c): a server creates a named port, a client
 * connects, the server accepts + completes the connection, and a request/reply
 * round trip is performed. The full message round trip is validated on ReactOS
 * (modern Win11 restricts the legacy LPC user-mode data path, so it cannot serve
 * as the oracle for the request/reply portion).
 */

#include "precomp.h"

#include <process.h>

#define LPC_MAX_DATA   0x130

typedef struct _LL_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} LL_MSG, *PLL_MSG;

typedef union _LL_BUFFER
{
    PORT_MESSAGE Header;
    UCHAR Raw[0x200];
} LL_BUFFER;

#define LL_MAX_REQUESTS 2

typedef struct _LL_CTX
{
    HANDLE ConnectionPort;
    NTSTATUS AcceptStatus;
    NTSTATUS CompleteStatus;
    NTSTATUS ReplyStatus;
    ULONG RequestValue[LL_MAX_REQUESTS];
    ULONG RequestType[LL_MAX_REQUESTS];
    ULONG RequestCount;
    ULONG DatagramValue;
    ULONG NotifyValue;
    BOOLEAN GotConnect;
    BOOLEAN GotDatagram;
    BOOLEAN GotNotify;
} LL_CTX, *PLL_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PLL_CTX Ctx = Parameter;
    LL_BUFFER Recv;
    HANDLE CommPort = NULL;
    PVOID PortContext = NULL;
    NTSTATUS Status;
    ULONG Type;

    for (;;)
    {
        RtlZeroMemory(&Recv, sizeof(Recv));
        Status = NtReplyWaitReceivePort(Ctx->ConnectionPort, &PortContext, NULL, &Recv.Header);
        if (!NT_SUCCESS(Status))
            break;

        Type = Recv.Header.u2.s2.Type & 0xFF;
        if (Type == LPC_CONNECTION_REQUEST)
        {
            Ctx->GotConnect = TRUE;
            Ctx->AcceptStatus = NtAcceptConnectPort(&CommPort, (PVOID)(ULONG_PTR)0x4321,
                                                    &Recv.Header, TRUE, NULL, NULL);
            if (!NT_SUCCESS(Ctx->AcceptStatus))
                break;
            Ctx->CompleteStatus = NtCompleteConnectPort(CommPort);
            if (!NT_SUCCESS(Ctx->CompleteStatus))
                break;
        }
        else if (Type == LPC_DATAGRAM)
        {
            /* An untyped NtRequestPort message arrives as LPC_DATAGRAM. */
            Ctx->GotDatagram = TRUE;
            Ctx->DatagramValue = ((PLL_MSG)&Recv)->Value;
        }
        else if (Type == LPC_CLIENT_DIED)
        {
            /* A pre-typed one-way message keeps its type on the wire (this is
             * how the kernel's thread-termination and hard-error messages reach
             * CSRSS with LPC_CLIENT_DIED / LPC_ERROR_EVENT intact). */
            Ctx->GotNotify = TRUE;
            Ctx->NotifyValue = ((PLL_MSG)&Recv)->Value;
        }
        else if (Type == LPC_REQUEST)
        {
            LL_MSG Reply;

            if (Ctx->RequestCount < LL_MAX_REQUESTS)
            {
                Ctx->RequestValue[Ctx->RequestCount] = ((PLL_MSG)&Recv)->Value;
                Ctx->RequestType[Ctx->RequestCount] = Recv.Header.u2.s2.Type;
            }
            Ctx->RequestCount++;

            RtlZeroMemory(&Reply, sizeof(Reply));
            Reply.Header = Recv.Header;
            Reply.Header.u1.s1.DataLength = sizeof(ULONG);
            Reply.Header.u1.s1.TotalLength = sizeof(LL_MSG);
            Reply.Value = 0xBEEF;
            Ctx->ReplyStatus = NtReplyPort(Ctx->ConnectionPort, &Reply.Header);
            if (!NT_SUCCESS(Ctx->ReplyStatus) || Ctx->RequestCount >= LL_MAX_REQUESTS)
                break;
        }
        else
        {
            /* PORT_CLOSED / other: done. */
            break;
        }
    }

    if (CommPort)
        NtClose(CommPort);
    return 0;
}

START_TEST(LegacyLpc)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort = NULL;
    HANDLE ClientPort = NULL;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE Qos;
    LL_CTX Ctx;
    LL_MSG Request;
    LL_BUFFER Reply;
    ULONG MaxMessageLength = 0;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    AlpcMakeUniquePortName(&PortName, NameBuffer, RTL_NUMBER_OF(NameBuffer));
    InitializeObjectAttributes(&ObjectAttributes, &PortName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtCreatePort(&ServerPort, &ObjectAttributes, 0x100, LPC_MAX_DATA, 0);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create legacy port\n");
        return;
    }

    RtlZeroMemory(&Ctx, sizeof(Ctx));
    Ctx.ConnectionPort = ServerPort;
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    RtlZeroMemory(&Qos, sizeof(Qos));
    Qos.Length = sizeof(Qos);
    Qos.ImpersonationLevel = SecurityImpersonation;
    Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    Qos.EffectiveOnly = FALSE;

    Status = NtConnectPort(&ClientPort, &PortName, &Qos, NULL, NULL,
                           &MaxMessageLength, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    trace("NtConnectPort: MaxMessageLength = %lu\n", MaxMessageLength);

    if (NT_SUCCESS(Status))
    {
        /* An untyped NtRequestPort send is delivered as LPC_DATAGRAM. */
        RtlZeroMemory(&Request, sizeof(Request));
        Request.Header.u1.s1.DataLength = sizeof(ULONG);
        Request.Header.u1.s1.TotalLength = sizeof(LL_MSG);
        Request.Value = 0x0DA7A;
        Status = NtRequestPort(ClientPort, &Request.Header);
        ok_hex(Status, STATUS_SUCCESS);

        /* User-mode senders cannot forge kernel notification types: a
         * LPC_CLIENT_DIED-typed one-way send is rejected (Win11 oracle;
         * only kernel-mode senders such as ps/dbgk/ex may pre-type their
         * termination / debug / hard-error messages). */
        RtlZeroMemory(&Request, sizeof(Request));
        Request.Header.u1.s1.DataLength = sizeof(ULONG);
        Request.Header.u1.s1.TotalLength = sizeof(LL_MSG);
        Request.Header.u2.s2.Type = LPC_CLIENT_DIED;
        Request.Value = 0xDEAD1;
        Status = NtRequestPort(ClientPort, &Request.Header);
        ok(Status == STATUS_INVALID_PARAMETER,
           "NtRequestPort(LPC_CLIENT_DIED) = 0x%lx, expected STATUS_INVALID_PARAMETER\n",
           Status);

        /* Types outside the datagram family are rejected on a one-way send
         * (Win11 oracle). */
        RtlZeroMemory(&Request, sizeof(Request));
        Request.Header.u1.s1.DataLength = sizeof(ULONG);
        Request.Header.u1.s1.TotalLength = sizeof(LL_MSG);
        Request.Header.u2.s2.Type = LPC_REQUEST;
        Request.Value = 0x1111;
        Status = NtRequestPort(ClientPort, &Request.Header);
        ok(Status == STATUS_INVALID_PARAMETER,
           "NtRequestPort(LPC_REQUEST) = 0x%lx, expected STATUS_INVALID_PARAMETER\n",
           Status);

        /* Plain synchronous round trip. */
        RtlZeroMemory(&Request, sizeof(Request));
        Request.Header.u1.s1.DataLength = sizeof(ULONG);
        Request.Header.u1.s1.TotalLength = sizeof(LL_MSG);
        Request.Value = 0x1234ABCD;
        RtlZeroMemory(&Reply, sizeof(Reply));

        Status = NtRequestWaitReplyPort(ClientPort, &Request.Header, &Reply.Header);
        ok_hex(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
        {
            ok(((PLL_MSG)&Reply)->Value == 0xBEEF,
               "reply value 0x%lx != 0xBEEF\n", ((PLL_MSG)&Reply)->Value);
            ok(Reply.Header.u1.s1.DataLength == sizeof(ULONG),
               "reply DataLength %u != %u\n",
               Reply.Header.u1.s1.DataLength, (ULONG)sizeof(ULONG));
            /* Legacy replies read LPC_REPLY. Verified against the Vista
             * reference: AlpcpReplyLegacySynchronousRequest stores the reply
             * with PortMessage.Type = 2 and the legacy receive path masks the
             * delivered type with 0xC00F. */
            ok((Reply.Header.u2.s2.Type & 0xFF) == LPC_REPLY,
               "reply Type = %x (base %x), expected LPC_REPLY\n",
               Reply.Header.u2.s2.Type, Reply.Header.u2.s2.Type & 0xFF);
        }

        /* A forged type on a waiting send is ignored: the message is delivered
         * as a plain LPC_REQUEST and round-trips normally (Win11 oracle). */
        RtlZeroMemory(&Request, sizeof(Request));
        Request.Header.u1.s1.DataLength = sizeof(ULONG);
        Request.Header.u1.s1.TotalLength = sizeof(LL_MSG);
        Request.Header.u2.s2.Type = LPC_CONNECTION_REQUEST;
        Request.Value = 0x2222;
        RtlZeroMemory(&Reply, sizeof(Reply));
        Status = NtRequestWaitReplyPort(ClientPort, &Request.Header, &Reply.Header);
        ok_hex(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
        {
            ok(((PLL_MSG)&Reply)->Value == 0xBEEF,
               "forged-type reply value 0x%lx != 0xBEEF\n", ((PLL_MSG)&Reply)->Value);
        }
    }

    if (ClientPort)
        NtClose(ClientPort);

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    ok(Ctx.GotConnect, "server did not receive a connection request\n");
    ok_hex(Ctx.AcceptStatus, STATUS_SUCCESS);
    ok_hex(Ctx.CompleteStatus, STATUS_SUCCESS);
    ok(Ctx.GotDatagram, "server did not receive the untyped datagram as LPC_DATAGRAM\n");
    ok(Ctx.DatagramValue == 0x0DA7A,
       "server saw datagram value 0x%lx != 0xDA7A\n", Ctx.DatagramValue);
    ok(!Ctx.GotNotify,
       "server received a forged LPC_CLIENT_DIED message (value 0x%lx)\n", Ctx.NotifyValue);
    ok(Ctx.RequestCount == 2,
       "server saw %lu requests, expected 2\n", Ctx.RequestCount);
    ok(Ctx.RequestValue[0] == 0x1234ABCD,
       "server saw request value 0x%lx != 0x1234ABCD\n", Ctx.RequestValue[0]);
    ok(Ctx.RequestValue[1] == 0x2222,
       "server saw forged-type request value 0x%lx != 0x2222\n", Ctx.RequestValue[1]);
    ok((Ctx.RequestType[1] & 0xFF) == LPC_REQUEST,
       "forged-type request arrived as type %lx (base %lx), expected LPC_REQUEST\n",
       Ctx.RequestType[1], Ctx.RequestType[1] & 0xFF);

    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

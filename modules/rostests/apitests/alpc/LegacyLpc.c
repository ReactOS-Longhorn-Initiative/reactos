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

typedef struct _LL_CTX
{
    HANDLE ConnectionPort;
    NTSTATUS AcceptStatus;
    NTSTATUS CompleteStatus;
    NTSTATUS ReplyStatus;
    ULONG RequestValue;
    BOOLEAN GotConnect;
    BOOLEAN GotRequest;
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
        else if (Type == LPC_REQUEST)
        {
            LL_MSG Reply;

            Ctx->GotRequest = TRUE;
            Ctx->RequestValue = ((PLL_MSG)&Recv)->Value;

            RtlZeroMemory(&Reply, sizeof(Reply));
            Reply.Header = Recv.Header;
            Reply.Header.u1.s1.DataLength = sizeof(ULONG);
            Reply.Header.u1.s1.TotalLength = sizeof(LL_MSG);
            Reply.Value = 0xBEEF;
            Ctx->ReplyStatus = NtReplyPort(Ctx->ConnectionPort, &Reply.Header);
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
        }
    }

    if (ClientPort)
        NtClose(ClientPort);

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    ok(Ctx.GotConnect, "server did not receive a connection request\n");
    ok_hex(Ctx.AcceptStatus, STATUS_SUCCESS);
    ok_hex(Ctx.CompleteStatus, STATUS_SUCCESS);
    ok(Ctx.GotRequest, "server did not receive the request\n");
    ok(Ctx.RequestValue == 0x1234ABCD,
       "server saw request value 0x%lx != 0x1234ABCD\n", Ctx.RequestValue);

    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

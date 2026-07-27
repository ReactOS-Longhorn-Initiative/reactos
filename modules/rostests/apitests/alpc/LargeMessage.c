/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Large messages round-trip up to the port's maximum length
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * A port's MaxMessageLength may be set anywhere up to the ABI maximum (0xFFEF);
 * the Win11 oracle rejects anything at or above 0x10000 with STATUS_INVALID_-
 * PARAMETER. A message that fills most of a generous port maximum must survive
 * the round trip with its payload intact. (Whether the kernel backs such a
 * message inline or with a section is an internal detail; the observable
 * behaviour is a faithful copy.)
 */

#include "precomp.h"

#include <process.h>

#define LARGE_PAYLOAD       0x4000
#define LARGE_PORT_MAXMSG   0x8000

typedef struct _LM_BUFFER
{
    PORT_MESSAGE Header;
    UCHAR Data[LARGE_PAYLOAD];
} LM_BUFFER, *PLM_BUFFER;

typedef struct _LM_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
    NTSTATUS RecvStatus;
    ULONG RecvDataLength;
    BOOLEAN PayloadOk;
} LM_CTX, *PLM_CTX;

static UCHAR PayloadByte(ULONG i) { return (UCHAR)(0xA5 ^ (i & 0xFF)); }

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PLM_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    PLM_BUFFER Recv;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;
    ULONG i;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Recv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LM_BUFFER));
    if (Recv == NULL)
        return 0;

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    ok_hex(Ctx->AcceptStatus, STATUS_SUCCESS);
    if (!NT_SUCCESS(Ctx->AcceptStatus))
    {
        HeapFree(GetProcessHeap(), 0, Recv);
        return 0;
    }

    BufferLength = sizeof(LM_BUFFER);
    Ctx->RecvStatus = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, 0, NULL, NULL,
                                                &Recv->Header, &BufferLength, NULL, &Timeout);
    ok_hex(Ctx->RecvStatus, STATUS_SUCCESS);
    if (NT_SUCCESS(Ctx->RecvStatus))
    {
        Ctx->RecvDataLength = Recv->Header.u1.s1.DataLength;
        Ctx->PayloadOk = TRUE;
        for (i = 0; i < LARGE_PAYLOAD; i++)
        {
            if (Recv->Data[i] != PayloadByte(i))
            {
                Ctx->PayloadOk = FALSE;
                break;
            }
        }

        /* Reply so the client's synchronous send returns. */
        Recv->Header.u1.s1.DataLength = sizeof(ULONG);
        Recv->Header.u1.s1.TotalLength = sizeof(PORT_MESSAGE) + sizeof(ULONG);
        NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, ALPC_MSGFLG_REPLY_MESSAGE,
                                  &Recv->Header, NULL, NULL, NULL, NULL, &Timeout);
    }

    HeapFree(GetProcessHeap(), 0, Recv);
    return 0;
}

START_TEST(LargeMessage)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    LM_CTX Ctx;
    PLM_BUFFER Request;
    ALPC_TEST_MESSAGE_BUFFER Reply;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;
    ULONG i;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    AlpcMakeUniquePortName(&PortName, NameBuffer, RTL_NUMBER_OF(NameBuffer));
    Status = AlpcCreateServerPort(&ServerPort, &PortName, LARGE_PORT_MAXMSG);
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
        Request = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LM_BUFFER));
        ok(Request != NULL, "failed to allocate request\n");
        if (Request)
        {
            AlpcInitMessageHeader(&Request->Header, (USHORT)LARGE_PAYLOAD);
            for (i = 0; i < LARGE_PAYLOAD; i++)
                Request->Data[i] = PayloadByte(i);

            RtlZeroMemory(&Reply, sizeof(Reply));
            BufferLength = sizeof(Reply);
            Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                               &Request->Header, NULL,
                                               &Reply.Header, &BufferLength, NULL, &Timeout);
            ok_hex(Status, STATUS_SUCCESS);
            HeapFree(GetProcessHeap(), 0, Request);
        }
    }

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    ok(Ctx.RecvDataLength == LARGE_PAYLOAD,
       "server received DataLength %lu, expected %u\n", Ctx.RecvDataLength, LARGE_PAYLOAD);
    ok(Ctx.PayloadOk, "large payload did not survive the round trip\n");

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

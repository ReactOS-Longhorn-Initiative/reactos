/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PORT_MESSAGE header validation on NtAlpcSendWaitReceivePort
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

#include <process.h>
#include <stdlib.h>

typedef struct _SRV_CTX
{
    HANDLE ConnectionPort;
    HANDLE StopEvent;
    NTSTATUS Status;
} SRV_CTX, *PSRV_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PSRV_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    HANDLE CommPort = NULL;

    Ctx->Status = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                      &CommPort, &ConnRequest);
    ok_hex(Ctx->Status, STATUS_SUCCESS);

    /* Hold the connection; the client only exercises send-side validation. */
    NtWaitForSingleObject(Ctx->StopEvent, FALSE, NULL);

    if (CommPort)
        NtClose(CommPort);
    return 0;
}

START_TEST(MessageValidation)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    SRV_CTX Ctx;
    PPORT_MESSAGE BigMsg;
    LARGE_INTEGER Timeout;
    const ULONG BigBufferSize = 0x8000;

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
    Ctx.ConnectionPort = ServerPort;
    Status = NtCreateEvent(&Ctx.StopEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    ok_hex(Status, STATUS_SUCCESS);

    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to connect\n");
        NtSetEvent(Ctx.StopEvent, NULL);
        goto Cleanup;
    }

    /* Large backing buffer so the kernel's copy of TotalLength bytes is valid. */
    BigMsg = malloc(BigBufferSize);
    ok(BigMsg != NULL, "malloc failed\n");
    if (BigMsg)
    {
        /* (1) TotalLength beyond the port maximum -> message too long. */
        RtlZeroMemory(BigMsg, sizeof(PORT_MESSAGE));
        BigMsg->u1.s1.TotalLength = 0x7000;
        BigMsg->u1.s1.DataLength = 0x7000 - sizeof(PORT_MESSAGE);
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, BigMsg, NULL,
                                           NULL, NULL, NULL, &Timeout);
        ok(Status == STATUS_PORT_MESSAGE_TOO_LONG,
           "oversize send = 0x%lx, expected STATUS_PORT_MESSAGE_TOO_LONG\n", Status);

        /*
         * (2) DataLength greater than TotalLength.
         *
         * The Win10 decompile's native-mode path requires
         * DataLength + sizeof(PORT_MESSAGE) == TotalLength and would reject this
         * with STATUS_INVALID_PARAMETER. The Win11 oracle, however, ACCEPTS it
         * (returns STATUS_SUCCESS) - it clamps the effective data to TotalLength
         * rather than failing. This field-leniency is version specific, so we
         * record the observed behavior instead of asserting a brittle code.
         */
        RtlZeroMemory(BigMsg, sizeof(PORT_MESSAGE));
        BigMsg->u1.s1.TotalLength = sizeof(PORT_MESSAGE);
        BigMsg->u1.s1.DataLength = 200;
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, BigMsg, NULL,
                                           NULL, NULL, NULL, &Timeout);
        trace("DataLength>TotalLength send = 0x%lx "
              "(Win11 accepts/clamps; Win10 returns STATUS_INVALID_PARAMETER)\n", Status);

        free(BigMsg);
    }

    NtSetEvent(Ctx.StopEvent, NULL);

Cleanup:
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);
    if (ClientCommPort)
        NtClose(ClientCommPort);
    NtClose(ThreadHandle);
    NtClose(Ctx.StopEvent);
    NtClose(ServerPort);
}

/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     A client port from an asynchronous connect stays ConnectionPending
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NtAlpcConnectPort without ALPC_MSGFLG_SYNC_REQUEST does not block for the
 * server's accept; it returns a client port that is still in the
 * ConnectionPending state. Sending on such a port is refused with
 * STATUS_LPC_REQUESTS_NOT_ALLOWED (the send-side state gate).
 */

#include "precomp.h"

typedef struct _PENDING_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} PENDING_MSG, *PPENDING_MSG;

START_TEST(ConnectPending)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    ALPC_PORT_ATTRIBUTES PortAttributes;
    ALPC_TEST_MESSAGE_BUFFER ConnMsg;
    SIZE_T ConnMsgLen = sizeof(ConnMsg);
    PENDING_MSG Request;
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

    /* Asynchronous connect: no SYNC_REQUEST and nobody accepts, so the call
     * returns immediately with a still-pending client port. */
    AlpcInitDefaultPortAttributes(&PortAttributes, ALPC_TEST_PORT_MAXMSG);
    RtlZeroMemory(&ConnMsg, sizeof(ConnMsg));
    AlpcInitMessageHeader(&ConnMsg.Header, 0);
    Status = NtAlpcConnectPort(&ClientCommPort, &PortName, NULL, &PortAttributes,
                               0, NULL, &ConnMsg.Header, &ConnMsgLen, NULL, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    ok(ClientCommPort != NULL, "ClientCommPort is NULL\n");

    if (NT_SUCCESS(Status) && ClientCommPort != NULL)
    {
        /* A send on the pending port must be refused. */
        Timeout.QuadPart = (LONGLONG)-1 * 1000 * 1000 * 10; /* 1s */
        RtlZeroMemory(&Request, sizeof(Request));
        AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
        Request.Value = 0x1234;
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, &Request.Header, NULL,
                                           NULL, NULL, NULL, &Timeout);
        ok(Status == STATUS_LPC_REQUESTS_NOT_ALLOWED,
           "send on pending port = 0x%lx, expected STATUS_LPC_REQUESTS_NOT_ALLOWED\n", Status);

        NtClose(ClientCommPort);
    }

    NtClose(ServerPort);
}

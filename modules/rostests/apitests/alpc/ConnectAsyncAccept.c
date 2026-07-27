/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Asynchronous connect: accept posts a connection reply on the client port
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NtAlpcConnectPort without ALPC_MSGFLG_SYNC_REQUEST returns immediately with a
 * still-pending client communication port while the connection request stays
 * queued for the server. Once the server accepts it, the kernel delivers the
 * connection reply on the CLIENT communication port with the internal message
 * type 11 (LPC_CONNECTION_REQUEST + 1): in the Vista reference,
 * AlpcpAcceptConnectPort dispatches DispatchContext.Type = 11 through
 * AlpcpDispatchReplyToPort into the client port's own queue when no thread is
 * blocked on the request. After collecting the reply the port is fully
 * connected and can send. A refused pending port reports
 * STATUS_PORT_CONNECTION_REFUSED from the send-side state gate.
 */

#include "precomp.h"

#define LPC_CONNECTION_REPLY (LPC_CONNECTION_REQUEST + 1)

typedef struct _CAA_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} CAA_MSG, *PCAA_MSG;

START_TEST(ConnectAsyncAccept)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort = NULL;
    HANDLE ServerCommPort = NULL;
    HANDLE ClientCommPort = NULL;
    HANDLE Client2CommPort = NULL;
    ALPC_PORT_ATTRIBUTES PortAttributes;
    CAA_MSG ConnMsg;
    SIZE_T ConnMsgLen;
    union { CAA_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    SIZE_T BufferLength;
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = (LONGLONG)-5 * 1000 * 1000 * 10; /* 5s */

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

    /* Asynchronous connect: returns at once with a pending client port; the
     * connection request stays queued for the server. */
    AlpcInitDefaultPortAttributes(&PortAttributes, ALPC_TEST_PORT_MAXMSG);
    RtlZeroMemory(&ConnMsg, sizeof(ConnMsg));
    AlpcInitMessageHeader(&ConnMsg.Header, sizeof(ULONG));
    ConnMsg.Value = 0xC0FFEE;
    ConnMsgLen = sizeof(ConnMsg);
    Status = NtAlpcConnectPort(&ClientCommPort, &PortName, NULL, &PortAttributes,
                               0, NULL, &ConnMsg.Header, &ConnMsgLen, NULL, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    ok(ClientCommPort != NULL, "ClientCommPort is NULL\n");
    if (!NT_SUCCESS(Status) || ClientCommPort == NULL)
    {
        skip("Async connect failed\n");
        goto Cleanup;
    }

    /* The server can collect the parked request even though the client never
     * blocked on it. */
    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(ServerPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status != STATUS_SUCCESS)
    {
        skip("Server did not receive the async connection request\n");
        goto Cleanup;
    }
    ok(ALPC_MSG_TYPE(Recv.Raw.Header) == LPC_CONNECTION_REQUEST,
       "conn request Type = %x (base %x), expected LPC_CONNECTION_REQUEST\n",
       Recv.Raw.Header.u2.s2.Type, ALPC_MSG_TYPE(Recv.Raw.Header));
    ok(Recv.Msg.Value == 0xC0FFEE,
       "conn request Value = 0x%lx, expected 0xC0FFEE\n", Recv.Msg.Value);

    Status = NtAlpcAcceptConnectPort(&ServerCommPort, ServerPort, 0, NULL,
                                     &PortAttributes, (PVOID)(ULONG_PTR)0x1111,
                                     &Recv.Raw.Header, NULL, TRUE);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Accept of async connection failed\n");
        goto Cleanup;
    }

    /* The accept must have posted the connection reply on the CLIENT port. */
    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        trace("connection reply raw Type = %x\n", Recv.Raw.Header.u2.s2.Type);
        ok(ALPC_MSG_TYPE(Recv.Raw.Header) == LPC_CONNECTION_REPLY,
           "conn reply Type = %x (base %x), expected LPC_CONNECTION_REPLY (11)\n",
           Recv.Raw.Header.u2.s2.Type, ALPC_MSG_TYPE(Recv.Raw.Header));
        ok(Recv.Raw.Header.u1.s1.DataLength == sizeof(ULONG),
           "conn reply DataLength = %u, expected %u\n",
           Recv.Raw.Header.u1.s1.DataLength, (ULONG)sizeof(ULONG));
        ok(Recv.Msg.Value == 0xC0FFEE,
           "conn reply Value = 0x%lx, expected the echoed 0xC0FFEE\n", Recv.Msg.Value);
    }

    /* The port has left ConnectionPending: a datagram send now succeeds... */
    RtlZeroMemory(&ConnMsg, sizeof(ConnMsg));
    AlpcInitMessageHeader(&ConnMsg.Header, sizeof(ULONG));
    ConnMsg.Value = 0xF00D;
    Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, &ConnMsg.Header, NULL,
                                       NULL, NULL, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);

    /* ...and reaches the server. */
    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Status = NtAlpcSendWaitReceivePort(ServerPort, 0, NULL, NULL,
                                       &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        ok(ALPC_MSG_TYPE(Recv.Raw.Header) == LPC_REQUEST,
           "post-connect msg Type = %x (base %x), expected LPC_REQUEST\n",
           Recv.Raw.Header.u2.s2.Type, ALPC_MSG_TYPE(Recv.Raw.Header));
        ok(Recv.Msg.Value == 0xF00D,
           "post-connect msg Value = 0x%lx, expected 0xF00D\n", Recv.Msg.Value);
    }

    /* Refusal path: a second async connect that the server refuses leaves the
     * client port refused; sending on it reports the refusal. */
    RtlZeroMemory(&ConnMsg, sizeof(ConnMsg));
    AlpcInitMessageHeader(&ConnMsg.Header, sizeof(ULONG));
    ConnMsg.Value = 0xB0DE;
    ConnMsgLen = sizeof(ConnMsg);
    Status = NtAlpcConnectPort(&Client2CommPort, &PortName, NULL, &PortAttributes,
                               0, NULL, &ConnMsg.Header, &ConnMsgLen, NULL, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status) && Client2CommPort != NULL)
    {
        RtlZeroMemory(&Recv, sizeof(Recv));
        BufferLength = sizeof(Recv);
        Status = NtAlpcSendWaitReceivePort(ServerPort, 0, NULL, NULL,
                                           &Recv.Raw.Header, &BufferLength, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);
        if (Status == STATUS_SUCCESS)
        {
            Status = NtAlpcAcceptConnectPort(NULL, ServerPort, 0, NULL,
                                             &PortAttributes, NULL,
                                             &Recv.Raw.Header, NULL, FALSE);
            ok_hex(Status, STATUS_SUCCESS);

            RtlZeroMemory(&ConnMsg, sizeof(ConnMsg));
            AlpcInitMessageHeader(&ConnMsg.Header, sizeof(ULONG));
            ConnMsg.Value = 0x1234;
            Status = NtAlpcSendWaitReceivePort(Client2CommPort, 0, &ConnMsg.Header, NULL,
                                               NULL, NULL, NULL, &Timeout);
            ok(Status == STATUS_PORT_CONNECTION_REFUSED,
               "send on refused port = 0x%lx, expected STATUS_PORT_CONNECTION_REFUSED\n",
               Status);
        }
    }

Cleanup:
    if (Client2CommPort)
        NtClose(Client2CommPort);
    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (ServerCommPort)
        NtClose(ServerCommPort);
    if (ServerPort)
        NtClose(ServerPort);
}

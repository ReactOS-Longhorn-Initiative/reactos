/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     The SECURITY message attribute carries a sender's context handle
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * A plain message delivers no SECURITY attribute. When the sender attaches a
 * security context (created with NtAlpcCreateSecurityContext) and marks the
 * SECURITY attribute valid, the receiver gets that context's opaque handle in
 * ALPC_SECURITY_ATTR.ContextHandle. Verified against the Win11 oracle, where the
 * received ContextHandle equals the handle the sender created.
 */

#include "precomp.h"

#include <process.h>

typedef struct _SA_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} SA_MSG, *PSA_MSG;

typedef struct _SA_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
    NTSTATUS RecvStatus;
    ULONG RecvValid;
    ALPC_HANDLE RecvContextHandle;
} SA_CTX, *PSA_CTX;

static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PSA_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;
    union { SA_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Recv;
    SA_MSG Reply;
    UCHAR RecvAttrBuffer[256];
    PALPC_MESSAGE_ATTRIBUTES RecvAttr = (PALPC_MESSAGE_ATTRIBUTES)RecvAttrBuffer;
    PALPC_SECURITY_ATTR RecvSecurity;
    SIZE_T BufferLength, Required;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    ok_hex(Ctx->AcceptStatus, STATUS_SUCCESS);
    if (!NT_SUCCESS(Ctx->AcceptStatus))
        return 0;

    Status = AlpcInitializeMessageAttribute(ALPC_MESSAGE_SECURITY_ATTRIBUTE, RecvAttr,
                                            sizeof(RecvAttrBuffer), &Required);
    ok_hex(Status, STATUS_SUCCESS);

    RtlZeroMemory(&Recv, sizeof(Recv));
    BufferLength = sizeof(Recv);
    Ctx->RecvStatus = NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, 0, NULL, NULL,
                                                &Recv.Raw.Header, &BufferLength, RecvAttr, &Timeout);
    ok_hex(Ctx->RecvStatus, STATUS_SUCCESS);
    if (NT_SUCCESS(Ctx->RecvStatus))
    {
        Ctx->RecvValid = RecvAttr->ValidAttributes;
        RecvSecurity = AlpcGetMessageAttribute(RecvAttr, ALPC_MESSAGE_SECURITY_ATTRIBUTE);
        if (RecvSecurity != NULL)
            Ctx->RecvContextHandle = RecvSecurity->ContextHandle;
    }

    /* Reply so the client's synchronous send returns. */
    RtlZeroMemory(&Reply, sizeof(Reply));
    Reply.Header = Recv.Raw.Header;
    Reply.Header.u1.s1.DataLength = sizeof(ULONG);
    Reply.Header.u1.s1.TotalLength = sizeof(SA_MSG);
    Reply.Value = 0;
    NtAlpcSendWaitReceivePort(Ctx->ConnectionPort, ALPC_MSGFLG_REPLY_MESSAGE,
                              &Reply.Header, NULL, NULL, NULL, NULL, &Timeout);
    return 0;
}

START_TEST(SecurityAttribute)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    SA_CTX Ctx;
    SA_MSG Request;
    union { SA_MSG Msg; ALPC_TEST_MESSAGE_BUFFER Raw; } Reply;
    ALPC_SECURITY_ATTR CreateSecurity;
    SECURITY_QUALITY_OF_SERVICE Qos;
    ALPC_HANDLE CreatedHandle = NULL;
    UCHAR SendAttrBuffer[256];
    PALPC_MESSAGE_ATTRIBUTES SendAttr = (PALPC_MESSAGE_ATTRIBUTES)SendAttrBuffer;
    PALPC_SECURITY_ATTR SendSecurity;
    SIZE_T BufferLength, Required;
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    if (!NtAlpcCreateSecurityContext)
    {
        skip("NtAlpcCreateSecurityContext not available\n");
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

    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        /* Create a security context on the client communication port. */
        RtlZeroMemory(&Qos, sizeof(Qos));
        Qos.Length = sizeof(Qos);
        Qos.ImpersonationLevel = SecurityImpersonation;
        Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Qos.EffectiveOnly = FALSE;
        RtlZeroMemory(&CreateSecurity, sizeof(CreateSecurity));
        CreateSecurity.QoS = &Qos;
        Status = NtAlpcCreateSecurityContext(ClientCommPort, 0, &CreateSecurity);
        ok_hex(Status, STATUS_SUCCESS);
        CreatedHandle = CreateSecurity.ContextHandle;
        ok(CreatedHandle != NULL, "security context handle is NULL\n");

        /* Send a request with the SECURITY attribute marked valid. */
        Status = AlpcInitializeMessageAttribute(ALPC_MESSAGE_SECURITY_ATTRIBUTE, SendAttr,
                                                sizeof(SendAttrBuffer), &Required);
        ok_hex(Status, STATUS_SUCCESS);
        SendSecurity = AlpcGetMessageAttribute(SendAttr, ALPC_MESSAGE_SECURITY_ATTRIBUTE);
        ok(SendSecurity != NULL, "send security attribute pointer is NULL\n");
        if (SendSecurity)
        {
            SendSecurity->Flags = 0;
            SendSecurity->QoS = &Qos;
            SendSecurity->ContextHandle = CreatedHandle;
        }
        SendAttr->ValidAttributes = ALPC_MESSAGE_SECURITY_ATTRIBUTE;

        RtlZeroMemory(&Request, sizeof(Request));
        AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
        Request.Value = 0x5EC0;
        RtlZeroMemory(&Reply, sizeof(Reply));
        BufferLength = sizeof(Reply);
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, ALPC_MSGFLG_SYNC_REQUEST,
                                           &Request.Header, SendAttr,
                                           &Reply.Raw.Header, &BufferLength, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);
    }

    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    trace("server SECURITY ValidAttributes = 0x%lx, ContextHandle = %p (created %p)\n",
          Ctx.RecvValid, Ctx.RecvContextHandle, CreatedHandle);

    /* The receiver must see the SECURITY attribute as valid and carrying the
     * exact handle the sender created. */
    ok(Ctx.RecvValid & ALPC_MESSAGE_SECURITY_ATTRIBUTE,
       "server did not receive a valid SECURITY attribute (valid=0x%lx)\n", Ctx.RecvValid);
    ok(Ctx.RecvContextHandle == CreatedHandle,
       "received ContextHandle %p != created %p\n", Ctx.RecvContextHandle, CreatedHandle);

    if (CreatedHandle && NtAlpcDeleteSecurityContext)
        NtAlpcDeleteSecurityContext(ClientCommPort, 0, CreatedHandle);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

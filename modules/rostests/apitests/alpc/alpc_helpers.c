/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared helpers for the ALPC behavioral conformance suite
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

ALPC_API Alpc;

#define ALPC_RESOLVE(Member, Name) \
    (*(FARPROC *)&Alpc.Member = GetProcAddress(NtdllHandle, Name))

BOOLEAN
AlpcInitApi(VOID)
{
    static BOOLEAN Loaded = FALSE;
    static BOOLEAN Result = FALSE;
    HMODULE NtdllHandle;

    if (Loaded)
        return Result;
    Loaded = TRUE;

    NtdllHandle = GetModuleHandleW(L"ntdll.dll");
    if (NtdllHandle == NULL)
        return FALSE;

    ALPC_RESOLVE(CreatePort, "NtAlpcCreatePort");
    ALPC_RESOLVE(ConnectPort, "NtAlpcConnectPort");
    ALPC_RESOLVE(ConnectPortEx, "NtAlpcConnectPortEx");
    ALPC_RESOLVE(AcceptConnectPort, "NtAlpcAcceptConnectPort");
    ALPC_RESOLVE(SendWaitReceivePort, "NtAlpcSendWaitReceivePort");
    ALPC_RESOLVE(DisconnectPort, "NtAlpcDisconnectPort");
    ALPC_RESOLVE(CancelMessage, "NtAlpcCancelMessage");
    ALPC_RESOLVE(QueryInformation, "NtAlpcQueryInformation");
    ALPC_RESOLVE(SetInformation, "NtAlpcSetInformation");
    ALPC_RESOLVE(CreateResourceReserve, "NtAlpcCreateResourceReserve");
    ALPC_RESOLVE(DeleteResourceReserve, "NtAlpcDeleteResourceReserve");
    ALPC_RESOLVE(CreateSecurityContext, "NtAlpcCreateSecurityContext");
    ALPC_RESOLVE(DeleteSecurityContext, "NtAlpcDeleteSecurityContext");
    ALPC_RESOLVE(RevokeSecurityContext, "NtAlpcRevokeSecurityContext");
    ALPC_RESOLVE(QueryInformationMessage, "NtAlpcQueryInformationMessage");
    ALPC_RESOLVE(CreatePortSection, "NtAlpcCreatePortSection");
    ALPC_RESOLVE(DeletePortSection, "NtAlpcDeletePortSection");
    ALPC_RESOLVE(CreateSectionView, "NtAlpcCreateSectionView");
    ALPC_RESOLVE(DeleteSectionView, "NtAlpcDeleteSectionView");
    ALPC_RESOLVE(OpenSenderProcess, "NtAlpcOpenSenderProcess");
    ALPC_RESOLVE(OpenSenderThread, "NtAlpcOpenSenderThread");
    ALPC_RESOLVE(ImpersonateClientOfPort, "NtAlpcImpersonateClientOfPort");
    ALPC_RESOLVE(GetHeaderSize, "AlpcGetHeaderSize");
    ALPC_RESOLVE(InitializeMessageAttribute, "AlpcInitializeMessageAttribute");
    ALPC_RESOLVE(GetMessageAttribute, "AlpcGetMessageAttribute");

    /* Core routines required for the suite to run at all. */
    Result = (Alpc.CreatePort && Alpc.ConnectPort && Alpc.AcceptConnectPort &&
              Alpc.SendWaitReceivePort && Alpc.DisconnectPort && Alpc.QueryInformation &&
              Alpc.CancelMessage && Alpc.GetHeaderSize &&
              Alpc.InitializeMessageAttribute && Alpc.GetMessageAttribute);
    return Result;
}

VOID
AlpcMakeUniquePortName(
    _Out_ PUNICODE_STRING PortName,
    _Out_writes_z_(BufferCch) PWSTR Buffer,
    _In_ SIZE_T BufferCch)
{
    static LONG Counter = 0;
    LONG Unique = InterlockedIncrement(&Counter);

    StringCchPrintfW(Buffer, BufferCch,
                     L"\\RPC Control\\ReactOS_AlpcApitest_%lu_%ld",
                     GetCurrentProcessId(), Unique);
    RtlInitUnicodeString(PortName, Buffer);
}

VOID
AlpcInitDefaultPortAttributes(
    _Out_ PALPC_PORT_ATTRIBUTES Attributes,
    _In_ SIZE_T MaxMessageLength)
{
    RtlZeroMemory(Attributes, sizeof(*Attributes));
    Attributes->Flags = 0;
    Attributes->MaxMessageLength = MaxMessageLength;
    Attributes->SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Attributes->SecurityQos.ImpersonationLevel = SecurityImpersonation;
    Attributes->SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    Attributes->SecurityQos.EffectiveOnly = FALSE;
}

VOID
AlpcInitMessageHeader(
    _Out_ PPORT_MESSAGE Message,
    _In_ USHORT DataLength)
{
    RtlZeroMemory(Message, sizeof(*Message));
    Message->u1.s1.DataLength = DataLength;
    Message->u1.s1.TotalLength = (CSHORT)(sizeof(PORT_MESSAGE) + DataLength);
}

NTSTATUS
AlpcCreateServerPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ SIZE_T MaxMessageLength)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    ALPC_PORT_ATTRIBUTES PortAttributes;

    InitializeObjectAttributes(&ObjectAttributes, PortName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    AlpcInitDefaultPortAttributes(&PortAttributes, MaxMessageLength);

    return NtAlpcCreatePort(PortHandle, &ObjectAttributes, &PortAttributes);
}

NTSTATUS
AlpcServerAcceptOne(
    _In_ HANDLE ConnectionPort,
    _In_ BOOLEAN Accept,
    _In_opt_ PVOID PortContext,
    _Out_ PHANDLE CommPortOut,
    _Out_ PALPC_TEST_MESSAGE_BUFFER ConnRequestOut)
{
    NTSTATUS Status;
    SIZE_T BufferLength = sizeof(*ConnRequestOut);
    LARGE_INTEGER Timeout;

    *CommPortOut = NULL;
    RtlZeroMemory(ConnRequestOut, sizeof(*ConnRequestOut));

    /* Bound the wait so a broken implementation cannot hang the suite. */
    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    /* Wait for the inbound connection request on the connection port. */
    Status = NtAlpcSendWaitReceivePort(ConnectionPort,
                                       0,
                                       NULL,
                                       NULL,
                                       &ConnRequestOut->Header,
                                       &BufferLength,
                                       NULL,
                                       &Timeout);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Accept (or refuse) the connection. */
    return NtAlpcAcceptConnectPort(CommPortOut,
                                   ConnectionPort,
                                   0,
                                   NULL,
                                   NULL,
                                   PortContext,
                                   &ConnRequestOut->Header,
                                   NULL,
                                   Accept);
}

NTSTATUS
AlpcGetCurrentUserSid(
    _Out_writes_bytes_(SidBufferLength) PSID SidBuffer,
    _In_ ULONG SidBufferLength)
{
    NTSTATUS Status;
    HANDLE Token;
    UCHAR Buffer[256];
    PTOKEN_USER UserInfo = (PTOKEN_USER)Buffer;
    ULONG ReturnLength;

    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY, &Token);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtQueryInformationToken(Token, TokenUser, Buffer, sizeof(Buffer), &ReturnLength);
    NtClose(Token);
    if (!NT_SUCCESS(Status))
        return Status;

    if (RtlLengthSid(UserInfo->User.Sid) > SidBufferLength)
        return STATUS_BUFFER_TOO_SMALL;

    return RtlCopySid(SidBufferLength, SidBuffer, UserInfo->User.Sid);
}

NTSTATUS
AlpcClientConnect(
    _In_ PUNICODE_STRING PortName,
    _Out_ PHANDLE CommPortOut,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    ALPC_PORT_ATTRIBUTES PortAttributes;
    ALPC_TEST_MESSAGE_BUFFER ConnMsg;
    SIZE_T ConnMsgLen = sizeof(ConnMsg);

    *CommPortOut = NULL;
    AlpcInitDefaultPortAttributes(&PortAttributes, ALPC_TEST_PORT_MAXMSG);

    /*
     * Synchronous connect: ALPC_MSGFLG_SYNC_REQUEST makes the call block until
     * the server accepts (or refuses). Without it the kernel returns a port
     * still in the ConnectionPending state, and any send on it fails with
     * STATUS_LPC_REQUESTS_NOT_ALLOWED. We also supply a connection-message
     * buffer so the accept reply can be received, which clears the pending bit.
     */
    RtlZeroMemory(&ConnMsg, sizeof(ConnMsg));
    AlpcInitMessageHeader(&ConnMsg.Header, 0);

    return NtAlpcConnectPort(CommPortOut,
                             PortName,
                             NULL,
                             &PortAttributes,
                             ALPC_MSGFLG_SYNC_REQUEST,
                             NULL,
                             &ConnMsg.Header,
                             &ConnMsgLen,
                             NULL,
                             NULL,
                             Timeout);
}

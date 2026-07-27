/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared helpers for the ALPC behavioral conformance suite
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NOTE: This suite is authored to compile and PASS on real Windows (Vista+),
 *       which is the behavioral oracle. The same binary is then run on ReactOS
 *       to measure conformance of the ALPC reimplementation.
 */

#ifndef _ALPC_APITEST_HELPERS_H_
#define _ALPC_APITEST_HELPERS_H_

/*
 * --------------------------------------------------------------------------
 * Delay-loaded ALPC API table.
 *
 * Every NtAlpc / Alpc routine is resolved from ntdll at runtime rather than
 * statically imported. This fully decouples the suite from the ReactOS
 * ntdll.spec (whose ALPC argument counts are unreliable, and whose entries may
 * be plain stubs before the syscalls are implemented). On the Windows oracle
 * all entries resolve; on ReactOS, missing ones are NULL and the affected test
 * skips. The macros below redirect each public name to its table slot, so test
 * call sites keep writing NtAlpcCreatePort(...) unchanged.
 * --------------------------------------------------------------------------
 */
typedef struct _ALPC_API
{
    NTSTATUS (NTAPI *CreatePort)(PHANDLE, POBJECT_ATTRIBUTES, PALPC_PORT_ATTRIBUTES);
    NTSTATUS (NTAPI *ConnectPort)(PHANDLE, PUNICODE_STRING, POBJECT_ATTRIBUTES,
                                  PALPC_PORT_ATTRIBUTES, ULONG, PSID, PPORT_MESSAGE,
                                  PSIZE_T, PALPC_MESSAGE_ATTRIBUTES,
                                  PALPC_MESSAGE_ATTRIBUTES, PLARGE_INTEGER);
    NTSTATUS (NTAPI *ConnectPortEx)(PHANDLE, POBJECT_ATTRIBUTES, POBJECT_ATTRIBUTES,
                                    PALPC_PORT_ATTRIBUTES, ULONG, PSECURITY_DESCRIPTOR,
                                    PPORT_MESSAGE, PSIZE_T, PALPC_MESSAGE_ATTRIBUTES,
                                    PALPC_MESSAGE_ATTRIBUTES, PLARGE_INTEGER);
    NTSTATUS (NTAPI *AcceptConnectPort)(PHANDLE, HANDLE, ULONG, POBJECT_ATTRIBUTES,
                                        PALPC_PORT_ATTRIBUTES, PVOID, PPORT_MESSAGE,
                                        PALPC_MESSAGE_ATTRIBUTES, BOOLEAN);
    NTSTATUS (NTAPI *SendWaitReceivePort)(HANDLE, ULONG, PPORT_MESSAGE,
                                          PALPC_MESSAGE_ATTRIBUTES, PPORT_MESSAGE,
                                          PSIZE_T, PALPC_MESSAGE_ATTRIBUTES, PLARGE_INTEGER);
    NTSTATUS (NTAPI *DisconnectPort)(HANDLE, ULONG);
    NTSTATUS (NTAPI *CancelMessage)(HANDLE, ULONG, PALPC_CONTEXT_ATTR);
    NTSTATUS (NTAPI *QueryInformation)(HANDLE, ALPC_PORT_INFORMATION_CLASS, PVOID, ULONG, PULONG);
    NTSTATUS (NTAPI *SetInformation)(HANDLE, ALPC_PORT_INFORMATION_CLASS, PVOID, ULONG);
    NTSTATUS (NTAPI *CreateResourceReserve)(HANDLE, ULONG, SIZE_T, PALPC_HANDLE);
    NTSTATUS (NTAPI *DeleteResourceReserve)(HANDLE, ULONG, ALPC_HANDLE);
    NTSTATUS (NTAPI *CreateSecurityContext)(HANDLE, ULONG, PALPC_SECURITY_ATTR);
    NTSTATUS (NTAPI *DeleteSecurityContext)(HANDLE, ULONG, ALPC_HANDLE);
    NTSTATUS (NTAPI *RevokeSecurityContext)(HANDLE, ULONG, ALPC_HANDLE);
    NTSTATUS (NTAPI *QueryInformationMessage)(HANDLE, PPORT_MESSAGE,
                                              ALPC_MESSAGE_INFORMATION_CLASS, PVOID, ULONG, PULONG);
    NTSTATUS (NTAPI *CreatePortSection)(HANDLE, ULONG, HANDLE, SIZE_T, PALPC_HANDLE, PSIZE_T);
    NTSTATUS (NTAPI *DeletePortSection)(HANDLE, ULONG, ALPC_HANDLE);
    NTSTATUS (NTAPI *CreateSectionView)(HANDLE, ULONG, PALPC_DATA_VIEW_ATTR);
    NTSTATUS (NTAPI *DeleteSectionView)(HANDLE, ULONG, PVOID);
    NTSTATUS (NTAPI *OpenSenderProcess)(PHANDLE, HANDLE, PPORT_MESSAGE, ULONG,
                                        ACCESS_MASK, POBJECT_ATTRIBUTES);
    NTSTATUS (NTAPI *OpenSenderThread)(PHANDLE, HANDLE, PPORT_MESSAGE, ULONG,
                                       ACCESS_MASK, POBJECT_ATTRIBUTES);
    NTSTATUS (NTAPI *ImpersonateClientOfPort)(HANDLE, PPORT_MESSAGE, PVOID);
    ULONG    (NTAPI *GetHeaderSize)(ULONG);
    NTSTATUS (NTAPI *InitializeMessageAttribute)(ULONG, PALPC_MESSAGE_ATTRIBUTES, SIZE_T, PSIZE_T);
    PVOID    (NTAPI *GetMessageAttribute)(PALPC_MESSAGE_ATTRIBUTES, ULONG);
} ALPC_API;

extern ALPC_API Alpc;

/* Resolve the ALPC table from ntdll. Returns FALSE if a core routine is absent. */
BOOLEAN AlpcInitApi(VOID);

#define NtAlpcCreatePort                Alpc.CreatePort
#define NtAlpcConnectPort               Alpc.ConnectPort
#define NtAlpcConnectPortEx             Alpc.ConnectPortEx
#define NtAlpcAcceptConnectPort         Alpc.AcceptConnectPort
#define NtAlpcSendWaitReceivePort       Alpc.SendWaitReceivePort
#define NtAlpcDisconnectPort            Alpc.DisconnectPort
#define NtAlpcCancelMessage             Alpc.CancelMessage
#define NtAlpcQueryInformation          Alpc.QueryInformation
#define NtAlpcSetInformation            Alpc.SetInformation
#define NtAlpcCreateResourceReserve     Alpc.CreateResourceReserve
#define NtAlpcDeleteResourceReserve     Alpc.DeleteResourceReserve
#define NtAlpcCreateSecurityContext     Alpc.CreateSecurityContext
#define NtAlpcDeleteSecurityContext     Alpc.DeleteSecurityContext
#define NtAlpcRevokeSecurityContext     Alpc.RevokeSecurityContext
#define NtAlpcQueryInformationMessage   Alpc.QueryInformationMessage
#define NtAlpcCreatePortSection         Alpc.CreatePortSection
#define NtAlpcDeletePortSection         Alpc.DeletePortSection
#define NtAlpcCreateSectionView         Alpc.CreateSectionView
#define NtAlpcDeleteSectionView         Alpc.DeleteSectionView
#define NtAlpcOpenSenderProcess         Alpc.OpenSenderProcess
#define NtAlpcOpenSenderThread          Alpc.OpenSenderThread
#define NtAlpcImpersonateClientOfPort   Alpc.ImpersonateClientOfPort
#define AlpcGetHeaderSize               Alpc.GetHeaderSize
#define AlpcInitializeMessageAttribute  Alpc.InitializeMessageAttribute
#define AlpcGetMessageAttribute         Alpc.GetMessageAttribute

/*
 * Extract the base LPC message type from a PORT_MESSAGE. The kernel packs flag
 * bits into the high part of the Type field (e.g. 0x300a for a connection
 * request), so the actual type lives in the low byte. See AlpcpSendMessage:
 * "CapturedMessage.u2.s2.Type = v9 & 0x7FFF" with dispatch via LOBYTE(Type).
 */
#define ALPC_MSG_TYPE(Header)   ((Header).u2.s2.Type & 0xFF)

/* A comfortable inline message size for tests (well under any port maximum). */
#define ALPC_TEST_MAX_MSG       1024

/* Default maximum message length we request when creating test ports. */
#define ALPC_TEST_PORT_MAXMSG   AlpcGetHeaderSize(0) + ALPC_TEST_MAX_MSG

/*
 * A receive buffer large enough for any test message plus its PORT_MESSAGE
 * header, sized in PORT_MESSAGE units so it is naturally aligned.
 */
typedef union _ALPC_TEST_MESSAGE_BUFFER
{
    PORT_MESSAGE Header;
    UCHAR Raw[ALPC_TEST_MAX_MSG];
} ALPC_TEST_MESSAGE_BUFFER, *PALPC_TEST_MESSAGE_BUFFER;

/* Build a unique \RPC Control port name for this process + call site. */
VOID
AlpcMakeUniquePortName(
    _Out_ PUNICODE_STRING PortName,
    _Out_writes_z_(BufferCch) PWSTR Buffer,
    _In_ SIZE_T BufferCch);

/* Zero and initialize a default ALPC_PORT_ATTRIBUTES with the given max length. */
VOID
AlpcInitDefaultPortAttributes(
    _Out_ PALPC_PORT_ATTRIBUTES Attributes,
    _In_ SIZE_T MaxMessageLength);

/* Zero a PORT_MESSAGE header and set Total/Data lengths for DataLength payload bytes. */
VOID
AlpcInitMessageHeader(
    _Out_ PPORT_MESSAGE Message,
    _In_ USHORT DataLength);

/* Create a named server connection port with default attributes. */
NTSTATUS
AlpcCreateServerPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ SIZE_T MaxMessageLength);

/*
 * Server side: block on the connection port until a connection request arrives,
 * then accept (or refuse) it. On accept, returns the server communication port.
 * ConnRequestOut receives a copy of the connection PORT_MESSAGE (needed to accept).
 */
NTSTATUS
AlpcServerAcceptOne(
    _In_ HANDLE ConnectionPort,
    _In_ BOOLEAN Accept,
    _In_opt_ PVOID PortContext,
    _Out_ PHANDLE CommPortOut,
    _Out_ PALPC_TEST_MESSAGE_BUFFER ConnRequestOut);

/*
 * Client side: synchronously connect to a named port. Blocks until the server
 * accepts or refuses. On success returns the client communication port.
 */
NTSTATUS
AlpcClientConnect(
    _In_ PUNICODE_STRING PortName,
    _Out_ PHANDLE CommPortOut,
    _In_opt_ PLARGE_INTEGER Timeout);

/* Copy the current process's user SID into the caller's buffer. */
NTSTATUS
AlpcGetCurrentUserSid(
    _Out_writes_bytes_(SidBufferLength) PSID SidBuffer,
    _In_ ULONG SidBufferLength);

#endif /* _ALPC_APITEST_HELPERS_H_ */

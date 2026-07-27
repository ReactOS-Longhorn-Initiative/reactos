/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Messages are delivered through a registered completion list
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * After a completion list is registered on a connection port, a datagram sent by
 * a client is written into the completion-list ring instead of the receive
 * queue. The server collects it with AlpcGetMessageFromCompletionList (ntdll),
 * inspects the payload and releases it with AlpcFreeCompletionListMessage.
 */

#include "precomp.h"

#include <process.h>

#define COMPLETION_BUFFER_SIZE 0x4000
#define DELIVERY_VALUE         0xCAFEF00D

typedef PVOID (NTAPI *PFN_GET_COMPLETION_MESSAGE)(PVOID, PVOID);
typedef BOOLEAN (NTAPI *PFN_FREE_COMPLETION_MESSAGE)(PVOID, PVOID);

typedef struct _CLD_MSG
{
    PORT_MESSAGE Header;
    ULONG Value;
} CLD_MSG, *PCLD_MSG;

typedef struct _CLD_CTX
{
    HANDLE ConnectionPort;
    HANDLE CommPort;
    NTSTATUS AcceptStatus;
} CLD_CTX, *PCLD_CTX;

/* Server thread: accept the connection, then exit (delivery is via the ring). */
static
UINT
CALLBACK
ServerThread(
    _Inout_ PVOID Parameter)
{
    PCLD_CTX Ctx = Parameter;
    ALPC_TEST_MESSAGE_BUFFER ConnRequest;

    Ctx->AcceptStatus = AlpcServerAcceptOne(Ctx->ConnectionPort, TRUE, NULL,
                                            &Ctx->CommPort, &ConnRequest);
    ok_hex(Ctx->AcceptStatus, STATUS_SUCCESS);
    return 0;
}

START_TEST(CompletionListDelivery)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    HANDLE ClientCommPort = NULL;
    HANDLE ThreadHandle;
    CLD_CTX Ctx;
    CLD_MSG Request;
    ALPC_HANDLE Section = NULL;
    SIZE_T ActualSize = 0;
    ALPC_DATA_VIEW_ATTR View;
    ALPC_PORT_COMPLETION_LIST_INFORMATION Info;
    PFN_GET_COMPLETION_MESSAGE pGetMessage;
    PFN_FREE_COMPLETION_MESSAGE pFreeMessage;
    PCLD_MSG Received = NULL;
    LARGE_INTEGER Timeout;
    HMODULE NtDll;
    ULONG Spin;

    Timeout.QuadPart = (LONGLONG)-10 * 1000 * 1000 * 10; /* 10s */

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    NtDll = GetModuleHandleW(L"ntdll.dll");
    pGetMessage = (PFN_GET_COMPLETION_MESSAGE)GetProcAddress(NtDll, "AlpcGetMessageFromCompletionList");
    pFreeMessage = (PFN_FREE_COMPLETION_MESSAGE)GetProcAddress(NtDll, "AlpcFreeCompletionListMessage");
    if (!pGetMessage || !pFreeMessage || !NtAlpcSetInformation ||
        !NtAlpcCreatePortSection || !NtAlpcCreateSectionView)
    {
        skip("Completion-list API not available\n");
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

    /* Build the completion-list buffer as a mapped section view. */
    Status = NtAlpcCreatePortSection(ServerPort, 0, NULL, COMPLETION_BUFFER_SIZE,
                                     &Section, &ActualSize);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create port section\n");
        NtClose(ServerPort);
        return;
    }

    RtlZeroMemory(&View, sizeof(View));
    View.SectionHandle = Section;
    View.ViewSize = ActualSize;
    Status = NtAlpcCreateSectionView(ServerPort, 0, &View);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create section view\n");
        NtAlpcDeletePortSection(ServerPort, 0, Section);
        NtClose(ServerPort);
        return;
    }

    RtlZeroMemory(&Info, sizeof(Info));
    Info.Buffer = View.ViewBase;
    Info.Size = (ULONG)View.ViewSize;
    Info.ConcurrencyCount = 1;
    Status = NtAlpcSetInformation(ServerPort, AlpcRegisterCompletionListInformation,
                                  &Info, sizeof(Info));
    ok_hex(Status, STATUS_SUCCESS);

    /* Accept a client connection (handshake is unaffected by the completion list). */
    RtlZeroMemory(&Ctx, sizeof(Ctx));
    Ctx.ConnectionPort = ServerPort;
    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, ServerThread, &Ctx, 0, NULL);
    ok(ThreadHandle != NULL, "_beginthreadex failed\n");

    Status = AlpcClientConnect(&PortName, &ClientCommPort, &Timeout);
    ok_hex(Status, STATUS_SUCCESS);
    NtWaitForSingleObject(ThreadHandle, FALSE, NULL);

    if (NT_SUCCESS(Status) && NT_SUCCESS(Ctx.AcceptStatus))
    {
        /* Datagram send -> routed into the completion-list ring. */
        RtlZeroMemory(&Request, sizeof(Request));
        AlpcInitMessageHeader(&Request.Header, sizeof(ULONG));
        Request.Value = DELIVERY_VALUE;
        Status = NtAlpcSendWaitReceivePort(ClientCommPort, 0, &Request.Header, NULL,
                                           NULL, NULL, NULL, &Timeout);
        ok_hex(Status, STATUS_SUCCESS);

        /* Collect the message from the ring (brief spin while it propagates). */
        for (Spin = 0; Spin < 1000 && Received == NULL; Spin++)
        {
            Received = (PCLD_MSG)pGetMessage(View.ViewBase, NULL);
            if (Received == NULL)
                Sleep(1);
        }

        ok(Received != NULL, "no message delivered to the completion list\n");
        if (Received != NULL)
        {
            ok(Received->Header.u1.s1.DataLength == sizeof(ULONG),
               "completion message DataLength %u != %u\n",
               Received->Header.u1.s1.DataLength, (ULONG)sizeof(ULONG));
            ok(Received->Value == DELIVERY_VALUE,
               "completion payload 0x%lx != 0x%lx\n", Received->Value, (ULONG)DELIVERY_VALUE);
            ok(pFreeMessage(View.ViewBase, Received),
               "AlpcFreeCompletionListMessage failed\n");
        }
    }

    NtAlpcSetInformation(ServerPort, AlpcUnregisterCompletionListInformation, NULL, 0);
    NtAlpcDeleteSectionView(ServerPort, 0, View.ViewBase);
    NtAlpcDeletePortSection(ServerPort, 0, Section);

    if (ClientCommPort)
        NtClose(ClientCommPort);
    if (Ctx.CommPort)
        NtClose(Ctx.CommPort);
    NtClose(ThreadHandle);
    NtClose(ServerPort);
}

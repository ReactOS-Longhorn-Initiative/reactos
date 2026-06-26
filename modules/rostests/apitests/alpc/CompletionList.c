/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for ALPC completion-list registration via NtAlpcSetInformation
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * AlpcRegisterCompletionListInformation requires a connection port (Type 1),
 * an info length of exactly sizeof(ALPC_PORT_COMPLETION_LIST_INFORMATION), and
 * a mapped section-view buffer the kernel initializes as the completion list.
 */

#include "precomp.h"

#define COMPLETION_BUFFER_SIZE 0x4000

START_TEST(CompletionList)
{
    NTSTATUS Status;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    HANDLE ServerPort;
    ALPC_HANDLE Section = NULL;
    SIZE_T ActualSize = 0;
    ALPC_DATA_VIEW_ATTR View;
    ALPC_PORT_COMPLETION_LIST_INFORMATION Info;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }
    if (!NtAlpcSetInformation || !NtAlpcCreatePortSection || !NtAlpcCreateSectionView ||
        !NtAlpcDeleteSectionView || !NtAlpcDeletePortSection)
    {
        skip("ALPC completion-list / section API not available\n");
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

    /* Allocate the completion-list buffer as a mapped section view. */
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

    /* Register the completion list. */
    RtlZeroMemory(&Info, sizeof(Info));
    Info.Buffer = View.ViewBase;
    Info.Size = (ULONG)View.ViewSize;
    Info.ConcurrencyCount = 1;
    Info.AttributeFlags = 0;

    Status = NtAlpcSetInformation(ServerPort, AlpcRegisterCompletionListInformation,
                                  &Info, sizeof(Info));
    ok_hex(Status, STATUS_SUCCESS);

    /* Wrong info length must be rejected. */
    Status = NtAlpcSetInformation(ServerPort, AlpcRegisterCompletionListInformation,
                                  &Info, sizeof(Info) - 4);
    ok(!NT_SUCCESS(Status),
       "register with wrong length unexpectedly succeeded (0x%lx)\n", Status);

    /* Unregister. */
    Status = NtAlpcSetInformation(ServerPort, AlpcUnregisterCompletionListInformation, NULL, 0);
    ok_hex(Status, STATUS_SUCCESS);

    NtAlpcDeleteSectionView(ServerPort, 0, View.ViewBase);
    NtAlpcDeletePortSection(ServerPort, 0, Section);
    NtClose(ServerPort);
}

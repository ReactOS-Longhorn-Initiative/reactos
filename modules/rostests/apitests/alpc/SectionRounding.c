/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NtAlpcCreatePortSection rounds the section size to 64 KB
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * The kernel rounds a requested section size up to the allocation granularity
 * (64 KB), not the page size, and reports the rounded value through
 * ActualSectionSize. This was verified against the Win11 oracle, where requests
 * of 1, 0x1000, 0x1001 and 0x1234 bytes all return an actual size of 0x10000.
 * A port section can be created on an unconnected connection port, so no
 * handshake is needed here.
 */

#include "precomp.h"

#define ALPC_TEST_GRANULARITY 0x10000

static
VOID
Test_OneSize(
    _In_ HANDLE PortHandle,
    _In_ SIZE_T Requested)
{
    NTSTATUS Status;
    ALPC_HANDLE Section = NULL;
    SIZE_T Actual = 0;
    SIZE_T Expected = (Requested + (ALPC_TEST_GRANULARITY - 1)) & ~(SIZE_T)(ALPC_TEST_GRANULARITY - 1);

    Status = NtAlpcCreatePortSection(PortHandle, 0, NULL, Requested, &Section, &Actual);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
        return;

    ok(Section != NULL, "Section handle is NULL for size %Iu\n", Requested);
    ok(Actual >= Requested, "Actual %Iu < requested %Iu\n", Actual, Requested);
    ok((Actual & (ALPC_TEST_GRANULARITY - 1)) == 0,
       "Actual %Iu is not 64KB-aligned (request %Iu)\n", Actual, Requested);
    ok(Actual == Expected,
       "Actual %Iu != expected %Iu (request %Iu)\n", Actual, Expected, Requested);

    NtAlpcDeletePortSection(PortHandle, 0, Section);
}

START_TEST(SectionRounding)
{
    NTSTATUS Status;
    HANDLE PortHandle;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    if (!NtAlpcCreatePortSection || !NtAlpcDeletePortSection)
    {
        skip("ALPC section API not available\n");
        return;
    }

    Status = NtAlpcCreatePort(&PortHandle, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create port\n");
        return;
    }

    Test_OneSize(PortHandle, 1);
    Test_OneSize(PortHandle, 0x1000);
    Test_OneSize(PortHandle, 0x1001);
    Test_OneSize(PortHandle, 0x1234);
    Test_OneSize(PortHandle, ALPC_TEST_GRANULARITY + 1);

    NtClose(PortHandle);
}

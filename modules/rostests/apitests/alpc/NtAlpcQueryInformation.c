/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcQueryInformation
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

START_TEST(NtAlpcQueryInformation)
{
    NTSTATUS Status;
    HANDLE PortHandle;
    ALPC_BASIC_INFORMATION BasicInfo;
    ULONG ReturnLength;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    Status = NtAlpcCreatePort(&PortHandle, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create port\n");
        return;
    }

    /* AlpcBasicInformation: well-formed query. */
    RtlFillMemory(&BasicInfo, sizeof(BasicInfo), 0x55);
    ReturnLength = 0;
    Status = NtAlpcQueryInformation(PortHandle, AlpcBasicInformation,
                                    &BasicInfo, sizeof(BasicInfo), &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(ReturnLength, (ULONG)sizeof(BasicInfo));
    trace("Flags=0x%lx SequenceNo=%lu PortContext=%p\n",
          BasicInfo.Flags, BasicInfo.SequenceNo, BasicInfo.PortContext);

    /* Invalid information class must be rejected. */
    ReturnLength = 0;
    Status = NtAlpcQueryInformation(PortHandle, (ALPC_PORT_INFORMATION_CLASS)0x7fffffff,
                                    &BasicInfo, sizeof(BasicInfo), &ReturnLength);
    trace("invalid class = 0x%lx\n", Status);
    ok(!NT_SUCCESS(Status),
       "Query with invalid class unexpectedly succeeded (0x%lx)\n", Status);

    /* Too-small buffer must be rejected, not silently truncated. */
    ReturnLength = 0;
    Status = NtAlpcQueryInformation(PortHandle, AlpcBasicInformation,
                                    &BasicInfo, 1, &ReturnLength);
    trace("undersized buffer = 0x%lx\n", Status);
    ok(!NT_SUCCESS(Status),
       "Query with undersized buffer unexpectedly succeeded (0x%lx)\n", Status);

    NtClose(PortHandle);
}

/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcCreateResourceReserve / NtAlpcDeleteResourceReserve
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

START_TEST(NtAlpcResourceReserve)
{
    NTSTATUS Status;
    HANDLE PortHandle;
    ALPC_HANDLE ReserveId;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }
    if (!NtAlpcCreateResourceReserve || !NtAlpcDeleteResourceReserve)
    {
        skip("ALPC resource-reserve API not available\n");
        return;
    }

    Status = NtAlpcCreatePort(&PortHandle, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create port\n");
        return;
    }

    /* Reserve quota for a 256-byte message. */
    ReserveId = NULL;
    Status = NtAlpcCreateResourceReserve(PortHandle, 0, 256, &ReserveId);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        ok(ReserveId != NULL, "ReserveId is NULL\n");

        Status = NtAlpcDeleteResourceReserve(PortHandle, 0, ReserveId);
        ok_hex(Status, STATUS_SUCCESS);
    }

    /* Deleting a non-existent reserve must fail. */
    Status = NtAlpcDeleteResourceReserve(PortHandle, 0, (ALPC_HANDLE)(ULONG_PTR)0x11223344);
    ok(!NT_SUCCESS(Status),
       "delete of a bogus reserve unexpectedly succeeded (0x%lx)\n", Status);

    NtClose(PortHandle);
}

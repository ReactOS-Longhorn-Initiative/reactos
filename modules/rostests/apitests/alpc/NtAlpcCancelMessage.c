/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcCancelMessage (parameter validation)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * NOTE: The full "cancel an in-flight pending message" behavior is timing
 *       sensitive and is covered in a later phase. Here we pin the
 *       deterministic, oracle-stable validation paths.
 */

#include "precomp.h"

START_TEST(NtAlpcCancelMessage)
{
    NTSTATUS Status;
    HANDLE EventHandle;
    HANDLE PortHandle;
    ALPC_CONTEXT_ATTR Context;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    RtlZeroMemory(&Context, sizeof(Context));
    Context.MessageId = 0xFFFFFFFF; /* a message that was never sent */

    /* Wrong object type: cancelling on a non-port handle must fail. */
    Status = NtCreateEvent(&EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    ok_hex(Status, STATUS_SUCCESS);

    Status = NtAlpcCancelMessage(EventHandle, 0, &Context);
    trace("CancelMessage(event handle) = 0x%lx\n", Status);
    ok(!NT_SUCCESS(Status),
       "CancelMessage on a non-port handle unexpectedly succeeded (0x%lx)\n", Status);
    NtClose(EventHandle);

    /* Valid port, but no such pending message: must not report success. */
    Status = NtAlpcCreatePort(&PortHandle, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        Status = NtAlpcCancelMessage(PortHandle, 0, &Context);
        trace("CancelMessage(no pending message) = 0x%lx\n", Status);
        ok(!NT_SUCCESS(Status),
           "CancelMessage of a non-existent message unexpectedly succeeded (0x%lx)\n", Status);
        NtClose(PortHandle);
    }
}

/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NtAlpcImpersonateClientOfPort gating (deterministic denials)
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * Impersonation is only permitted for a message sitting in the port's pending
 * queue. A message id that was never received, and a handle that is not an ALPC
 * port, are both refused. (The successful path additionally requires the
 * connection port to allow impersonation, which the default attributes do not -
 * confirmed against the Win11 oracle, so it is not asserted here.)
 */

#include "precomp.h"

START_TEST(ImpersonateDenied)
{
    NTSTATUS Status;
    HANDLE PortHandle;
    HANDLE EventHandle;
    union { PORT_MESSAGE Header; UCHAR Raw[64]; } FakeMessage;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    if (!NtAlpcImpersonateClientOfPort)
    {
        skip("NtAlpcImpersonateClientOfPort not available\n");
        return;
    }

    Status = NtAlpcCreatePort(&PortHandle, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create port\n");
        return;
    }

    /* A message id that is not in the pending queue must be refused. */
    RtlZeroMemory(&FakeMessage, sizeof(FakeMessage));
    FakeMessage.Header.u1.s1.TotalLength = sizeof(PORT_MESSAGE);
    FakeMessage.Header.MessageId = 0xDEADBEEF;
    Status = NtAlpcImpersonateClientOfPort(PortHandle, &FakeMessage.Header, NULL);
    ok(Status == STATUS_ACCESS_DENIED,
       "impersonate of an unknown message = 0x%lx, expected STATUS_ACCESS_DENIED\n", Status);

    /* A non-port handle must be rejected. */
    Status = NtCreateEvent(&EventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        Status = NtAlpcImpersonateClientOfPort(EventHandle, &FakeMessage.Header, NULL);
        ok(!NT_SUCCESS(Status),
           "impersonate on a non-port handle unexpectedly succeeded (0x%lx)\n", Status);
        NtClose(EventHandle);
    }

    NtClose(PortHandle);
}

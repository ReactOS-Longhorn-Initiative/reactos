/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcCreate/Delete/RevokeSecurityContext
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

static
VOID
InitQos(
    _Out_ PSECURITY_QUALITY_OF_SERVICE Qos)
{
    RtlZeroMemory(Qos, sizeof(*Qos));
    Qos->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Qos->ImpersonationLevel = SecurityImpersonation;
    Qos->ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    Qos->EffectiveOnly = FALSE;
}

START_TEST(NtAlpcSecurityContext)
{
    NTSTATUS Status;
    HANDLE PortHandle;
    ALPC_SECURITY_ATTR SecAttr;
    SECURITY_QUALITY_OF_SERVICE Qos;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }
    if (!NtAlpcCreateSecurityContext || !NtAlpcDeleteSecurityContext ||
        !NtAlpcRevokeSecurityContext)
    {
        skip("ALPC security-context API not available\n");
        return;
    }

    Status = NtAlpcCreatePort(&PortHandle, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create port\n");
        return;
    }

    /* Create a security context and delete it. */
    InitQos(&Qos);
    RtlZeroMemory(&SecAttr, sizeof(SecAttr));
    SecAttr.Flags = 0;
    SecAttr.QoS = &Qos;
    SecAttr.ContextHandle = NULL;

    Status = NtAlpcCreateSecurityContext(PortHandle, 0, &SecAttr);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        ok(SecAttr.ContextHandle != NULL, "ContextHandle is NULL\n");
        Status = NtAlpcDeleteSecurityContext(PortHandle, 0, SecAttr.ContextHandle);
        ok_hex(Status, STATUS_SUCCESS);
    }

    /* Create a second context, revoke it, then delete it. */
    InitQos(&Qos);
    RtlZeroMemory(&SecAttr, sizeof(SecAttr));
    SecAttr.QoS = &Qos;

    Status = NtAlpcCreateSecurityContext(PortHandle, 0, &SecAttr);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        Status = NtAlpcRevokeSecurityContext(PortHandle, 0, SecAttr.ContextHandle);
        ok_hex(Status, STATUS_SUCCESS);

        Status = NtAlpcDeleteSecurityContext(PortHandle, 0, SecAttr.ContextHandle);
        ok_hex(Status, STATUS_SUCCESS);
    }

    NtClose(PortHandle);
}

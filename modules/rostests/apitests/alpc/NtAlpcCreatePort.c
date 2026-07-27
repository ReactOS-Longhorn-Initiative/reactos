/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for NtAlpcCreatePort
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "precomp.h"

static
VOID
Test_Anonymous(VOID)
{
    NTSTATUS Status;
    HANDLE PortHandle;
    ALPC_PORT_ATTRIBUTES PortAttributes;

    /* Fully anonymous connection port: no object attributes, no port attributes. */
    PortHandle = NULL;
    Status = NtAlpcCreatePort(&PortHandle, NULL, NULL);
    ok_hex(Status, STATUS_SUCCESS);
    ok(PortHandle != NULL, "PortHandle is NULL\n");
    if (NT_SUCCESS(Status))
    {
        Status = NtClose(PortHandle);
        ok_hex(Status, STATUS_SUCCESS);
    }

    /* Anonymous port with explicit default attributes. */
    AlpcInitDefaultPortAttributes(&PortAttributes, ALPC_TEST_PORT_MAXMSG);
    PortHandle = NULL;
    Status = NtAlpcCreatePort(&PortHandle, NULL, &PortAttributes);
    ok_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
        NtClose(PortHandle);
}

static
VOID
Test_Named(VOID)
{
    NTSTATUS Status;
    HANDLE PortHandle, PortHandle2;
    UNICODE_STRING PortName;
    WCHAR NameBuffer[128];
    OBJECT_ATTRIBUTES ObjectAttributes;
    ALPC_PORT_ATTRIBUTES PortAttributes;
    ALPC_BASIC_INFORMATION BasicInfo;
    ULONG ReturnLength;

    AlpcMakeUniquePortName(&PortName, NameBuffer, RTL_NUMBER_OF(NameBuffer));
    InitializeObjectAttributes(&ObjectAttributes, &PortName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    AlpcInitDefaultPortAttributes(&PortAttributes, ALPC_TEST_PORT_MAXMSG);

    Status = NtAlpcCreatePort(&PortHandle, &ObjectAttributes, &PortAttributes);
    ok_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        skip("Failed to create named port\n");
        return;
    }

    /* Query the basic information back. */
    RtlZeroMemory(&BasicInfo, sizeof(BasicInfo));
    ReturnLength = 0;
    Status = NtAlpcQueryInformation(PortHandle, AlpcBasicInformation,
                                    &BasicInfo, sizeof(BasicInfo), &ReturnLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(ReturnLength, (ULONG)sizeof(BasicInfo));
    trace("BasicInfo.Flags=0x%lx SequenceNo=%lu\n", BasicInfo.Flags, BasicInfo.SequenceNo);

    /* Creating a second port with the same name must collide. */
    Status = NtAlpcCreatePort(&PortHandle2, &ObjectAttributes, &PortAttributes);
    ok_hex(Status, STATUS_OBJECT_NAME_COLLISION);
    if (NT_SUCCESS(Status))
        NtClose(PortHandle2);

    Status = NtClose(PortHandle);
    ok_hex(Status, STATUS_SUCCESS);
}

static
VOID
Test_InvalidParameters(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ALPC_PORT_ATTRIBUTES PortAttributes;

    /*
     * NULL output handle pointer must be rejected (the kernel probes the
     * argument and returns STATUS_ACCESS_VIOLATION rather than raising).
     */
    AlpcInitDefaultPortAttributes(&PortAttributes, ALPC_TEST_PORT_MAXMSG);
    StartSeh()
        Status = NtAlpcCreatePort(NULL, NULL, &PortAttributes);
    EndSeh(STATUS_SUCCESS);
    ok(Status == STATUS_ACCESS_VIOLATION,
       "NtAlpcCreatePort(NULL out) = 0x%lx, expected STATUS_ACCESS_VIOLATION\n", Status);
}

START_TEST(NtAlpcCreatePort)
{
    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    Test_Anonymous();
    Test_Named();
    Test_InvalidParameters();
}

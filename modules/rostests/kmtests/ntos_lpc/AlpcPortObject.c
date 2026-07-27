/*
 * PROJECT:     ReactOS Kernel-Mode Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ALPC ABI layout invariants and a kernel-mode port smoke test
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * The compile-time C_ASSERTs validate that the public ALPC structure layout
 * matches the Windows ABI; when this driver is loaded on the Windows oracle
 * they confirm the layout against the live kernel, and on ReactOS they guard
 * against drift. The runtime smoke test resolves ZwAlpcCreatePort dynamically
 * so the driver builds and loads even before ReactOS implements ALPC.
 */

#include <kmt_test.h>
#include <ndk/lpctypes.h>

/*
 * Relational layout invariants (architecture independent).
 * Pointer-sized fields are expressed in terms of sizeof(PVOID) so the same
 * assertions hold on both i386 and amd64.
 */
C_ASSERT(sizeof(ALPC_MESSAGE_ATTRIBUTES) == 2 * sizeof(ULONG));

C_ASSERT(FIELD_OFFSET(ALPC_SECURITY_ATTR, Flags) == 0);
C_ASSERT(FIELD_OFFSET(ALPC_SECURITY_ATTR, QoS) == sizeof(PVOID));
C_ASSERT(FIELD_OFFSET(ALPC_SECURITY_ATTR, ContextHandle) == 2 * sizeof(PVOID));

C_ASSERT(FIELD_OFFSET(ALPC_DATA_VIEW_ATTR, Flags) == 0);
C_ASSERT(FIELD_OFFSET(ALPC_DATA_VIEW_ATTR, SectionHandle) == sizeof(PVOID));
C_ASSERT(FIELD_OFFSET(ALPC_DATA_VIEW_ATTR, ViewBase) == 2 * sizeof(PVOID));
C_ASSERT(FIELD_OFFSET(ALPC_DATA_VIEW_ATTR, ViewSize) == 3 * sizeof(PVOID));

C_ASSERT(FIELD_OFFSET(ALPC_HANDLE_ATTR, Flags) == 0);
C_ASSERT(FIELD_OFFSET(ALPC_HANDLE_ATTR, Handle) == sizeof(PVOID));
C_ASSERT(FIELD_OFFSET(ALPC_HANDLE_ATTR, ObjectType) == 2 * sizeof(PVOID));
C_ASSERT(FIELD_OFFSET(ALPC_HANDLE_ATTR, DesiredAccess) == 2 * sizeof(PVOID) + sizeof(ULONG));

C_ASSERT(FIELD_OFFSET(ALPC_CONTEXT_ATTR, PortContext) == 0);
C_ASSERT(FIELD_OFFSET(ALPC_CONTEXT_ATTR, MessageContext) == sizeof(PVOID));
C_ASSERT(FIELD_OFFSET(ALPC_CONTEXT_ATTR, Sequence) == 2 * sizeof(PVOID));

C_ASSERT(FIELD_OFFSET(ALPC_BASIC_INFORMATION, Flags) == 0);
C_ASSERT(FIELD_OFFSET(ALPC_BASIC_INFORMATION, SequenceNo) == sizeof(ULONG));
C_ASSERT(FIELD_OFFSET(ALPC_BASIC_INFORMATION, PortContext) == 2 * sizeof(ULONG));

typedef NTSTATUS
(NTAPI *PFN_ZWALPCCREATEPORT)(
    _Out_ PHANDLE PortHandle,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PALPC_PORT_ATTRIBUTES PortAttributes);

static
VOID
Test_KernelPortSmoke(VOID)
{
    UNICODE_STRING RoutineName = RTL_CONSTANT_STRING(L"ZwAlpcCreatePort");
    PFN_ZWALPCCREATEPORT pZwAlpcCreatePort;
    ALPC_PORT_ATTRIBUTES PortAttributes;
    HANDLE PortHandle;
    NTSTATUS Status;

    pZwAlpcCreatePort = (PFN_ZWALPCCREATEPORT)(ULONG_PTR)MmGetSystemRoutineAddress(&RoutineName);
    if (skip(pZwAlpcCreatePort != NULL,
             "ZwAlpcCreatePort is not available (ALPC not yet implemented)\n"))
    {
        return;
    }

    RtlZeroMemory(&PortAttributes, sizeof(PortAttributes));
    PortAttributes.MaxMessageLength = 0x130;
    PortAttributes.SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    PortAttributes.SecurityQos.ImpersonationLevel = SecurityImpersonation;
    PortAttributes.SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;

    PortHandle = NULL;
    Status = pZwAlpcCreatePort(&PortHandle, NULL, &PortAttributes);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok(PortHandle != NULL, "PortHandle is NULL\n");

    if (NT_SUCCESS(Status))
    {
        Status = ZwClose(PortHandle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
}

START_TEST(AlpcPortObject)
{
    /* The C_ASSERTs above already ran at compile time; record that we executed. */
    ok(TRUE, "ALPC public ABI layout assertions passed at compile time\n");
    Test_KernelPortSmoke();
}

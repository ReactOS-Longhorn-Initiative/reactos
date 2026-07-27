/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for the ntdll ALPC message-attribute helper routines
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * These routines are pure user-mode math (no syscall), so they are fully
 * deterministic and are the easiest tests to certify against the Windows oracle.
 */

#include "precomp.h"

#include <stdlib.h>

static
SIZE_T
RequiredSizeFor(
    _In_ ULONG Flags)
{
    NTSTATUS Status;
    SIZE_T Required = 0;

    Status = AlpcInitializeMessageAttribute(Flags, NULL, 0, &Required);
    ok(Status == STATUS_BUFFER_TOO_SMALL,
       "AlpcInitializeMessageAttribute(0x%lx, NULL, 0) = 0x%lx, expected STATUS_BUFFER_TOO_SMALL\n",
       Flags, Status);
    return Required;
}

START_TEST(AlpcHelpers)
{
    NTSTATUS Status;
    ULONG HeaderSize;
    SIZE_T ReqSec, ReqSecView, ReqAll, Required;
    PALPC_MESSAGE_ATTRIBUTES Buffer;
    PVOID Attr, Attr2;

    if (!AlpcInitApi())
    {
        skip("ALPC API not available\n");
        return;
    }

    /* AlpcGetHeaderSize: must be non-zero and sane. */
    HeaderSize = AlpcGetHeaderSize(0);
    trace("AlpcGetHeaderSize(0) = %lu\n", HeaderSize);
    ok(HeaderSize != 0, "AlpcGetHeaderSize(0) returned 0\n");
    ok(HeaderSize < 0x1000, "AlpcGetHeaderSize(0) = %lu, unexpectedly large\n", HeaderSize);

    /* Required buffer sizes must grow monotonically with more attributes. */
    ReqSec = RequiredSizeFor(ALPC_MESSAGE_SECURITY_ATTRIBUTE);
    ReqSecView = RequiredSizeFor(ALPC_MESSAGE_SECURITY_ATTRIBUTE | ALPC_MESSAGE_VIEW_ATTRIBUTE);
    ReqAll = RequiredSizeFor(ALPC_MESSAGE_SECURITY_ATTRIBUTE | ALPC_MESSAGE_VIEW_ATTRIBUTE |
                             ALPC_MESSAGE_CONTEXT_ATTRIBUTE | ALPC_MESSAGE_HANDLE_ATTRIBUTE);

    trace("Required: sec=%Iu sec|view=%Iu all=%Iu\n", ReqSec, ReqSecView, ReqAll);
    ok(ReqSec >= sizeof(ALPC_MESSAGE_ATTRIBUTES),
       "ReqSec = %Iu, expected >= %Iu\n", ReqSec, sizeof(ALPC_MESSAGE_ATTRIBUTES));
    ok(ReqSecView > ReqSec, "ReqSecView = %Iu not > ReqSec = %Iu\n", ReqSecView, ReqSec);
    ok(ReqAll > ReqSecView, "ReqAll = %Iu not > ReqSecView = %Iu\n", ReqAll, ReqSecView);

    /* Initialize a real buffer for all four attributes. */
    Buffer = malloc(ReqAll);
    ok(Buffer != NULL, "malloc failed\n");
    if (!Buffer)
        return;
    RtlZeroMemory(Buffer, ReqAll);

    Required = 0;
    Status = AlpcInitializeMessageAttribute(
                 ALPC_MESSAGE_SECURITY_ATTRIBUTE | ALPC_MESSAGE_VIEW_ATTRIBUTE |
                 ALPC_MESSAGE_CONTEXT_ATTRIBUTE | ALPC_MESSAGE_HANDLE_ATTRIBUTE,
                 Buffer, ReqAll, &Required);
    ok_hex(Status, STATUS_SUCCESS);
    ok_eq_size(Required, ReqAll);

    /* AllocatedAttributes must reflect exactly what we asked for. */
    ok_eq_hex(Buffer->AllocatedAttributes,
              ALPC_MESSAGE_SECURITY_ATTRIBUTE | ALPC_MESSAGE_VIEW_ATTRIBUTE |
              ALPC_MESSAGE_CONTEXT_ATTRIBUTE | ALPC_MESSAGE_HANDLE_ATTRIBUTE);

    /*
     * AlpcGetMessageAttribute returns the in-buffer pointer for any attribute
     * present in AllocatedAttributes (confirmed against the Windows oracle: it
     * does NOT consult ValidAttributes).
     */
    Attr = AlpcGetMessageAttribute(Buffer, ALPC_MESSAGE_CONTEXT_ATTRIBUTE);
    ok(Attr != NULL, "context attribute pointer is NULL\n");
    ok((PUCHAR)Attr >= (PUCHAR)Buffer && (PUCHAR)Attr < (PUCHAR)Buffer + ReqAll,
       "context attribute %p outside buffer [%p, %p)\n",
       Attr, Buffer, (PUCHAR)Buffer + ReqAll);

    Attr2 = AlpcGetMessageAttribute(Buffer, ALPC_MESSAGE_SECURITY_ATTRIBUTE);
    ok(Attr2 != NULL, "security attribute pointer is NULL\n");
    ok(Attr2 != Attr, "distinct attributes returned the same pointer %p\n", Attr);

    /*
     * Re-initialize with only the security attribute allocated; querying an
     * attribute that is NOT in AllocatedAttributes must return NULL.
     */
    Required = 0;
    Status = AlpcInitializeMessageAttribute(ALPC_MESSAGE_SECURITY_ATTRIBUTE,
                                            Buffer, ReqAll, &Required);
    ok_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(Buffer->AllocatedAttributes, ALPC_MESSAGE_SECURITY_ATTRIBUTE);
    Attr = AlpcGetMessageAttribute(Buffer, ALPC_MESSAGE_HANDLE_ATTRIBUTE);
    ok(Attr == NULL, "handle attribute should be NULL when not allocated, got %p\n", Attr);

    free(Buffer);
}

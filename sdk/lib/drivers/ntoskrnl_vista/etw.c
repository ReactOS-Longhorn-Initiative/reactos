/*
 * PROJECT:     ReactOS Kernel - Vista+ APIs
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Etw functions of Vista+
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>

NTSTATUS
NTKRNLVISTAAPI
NTAPI
EtwActivityIdControl(
  _In_ ULONG ControlCode,
  _Inout_ LPGUID ActivityId
)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(HIGH_LEVEL)
NTSTATUS
NTKRNLVISTAAPI
NTAPI
EtwWrite(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_opt_ LPCGUID ActivityId,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(HIGH_LEVEL)
NTSTATUS
NTKRNLVISTAAPI
NTAPI
EtwWriteTransfer(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_opt_ LPCGUID ActivityId,
    _In_opt_ LPCGUID RelatedActivityId,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTKRNLVISTAAPI
NTAPI
EtwRegister(
    _In_ LPCGUID ProviderId,
    _In_opt_ PETWENABLECALLBACK EnableCallback,
    _In_opt_ PVOID CallbackContext,
    _Out_ PREGHANDLE RegHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTKRNLVISTAAPI
NTAPI
EtwUnregister(
    _In_ REGHANDLE RegHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
NTKRNLVISTAAPI
NTAPI
EtwProviderEnabled(
    _In_  REGHANDLE RegHandle,
    _In_  UCHAR     Level,
    _In_  ULONGLONG Keyword
)
{
    UNIMPLEMENTED;
    return FALSE;
}

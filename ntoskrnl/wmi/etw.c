/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/wmi/etc.c
 * PURPOSE:         I/O Etw Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define INITGUID
#include <wmidata.h>
#include <wmiguid.h>
#include <wmistr.h>

#include "wmip.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
EtwRegister(
    IN LPCGUID ProviderId,
    IN OPTIONAL PETWENABLECALLBACK EnableCallback,
    IN OPTIONAL PVOID CallbackContext,
    OUT PREGHANDLE RegHandle)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
EtwUnregister(
    IN REGHANDLE RegHandle)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
EtwWrite(
    IN REGHANDLE RegHandle,
    IN PCEVENT_DESCRIPTOR EventDescriptor,
    IN OPTIONAL LPCGUID ActivityId,
    IN ULONG UserDataCount,
    IN PEVENT_DATA_DESCRIPTOR UserData)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

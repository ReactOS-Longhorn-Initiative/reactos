/*
 * PROJECT:     ReactOS Kernel - Vista+ APIs
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Po functions of Vista+
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>

NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoRegisterPowerSettingCallback(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Outptr_opt_ PVOID *Handle)
{
    UNIMPLEMENTED;
    return STATUS_INSUFFICIENT_RESOURCES;
}

_IRQL_requires_max_(APC_LEVEL)
NTKRNLVISTAAPI
NTSTATUS
NTAPI
PoUnregisterPowerSettingCallback(
    _Inout_ PVOID Handle)
{
    UNIMPLEMENTED;
    return STATUS_INVALID_PARAMETER;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
BOOLEAN
NTAPI
PoQueryWatchdogTime(
    _In_ PDEVICE_OBJECT Pdo,
    _Out_ PULONG SecondsRemaining)
{
    *SecondsRemaining = 0;

    UNIMPLEMENTED;
    return FALSE;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
VOID
NTAPI
PoSetSystemWake(
    _Inout_ struct _IRP *Irp)
{
    UNIMPLEMENTED;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKRNLVISTAAPI
BOOLEAN
NTAPI
PoGetSystemWake(
    _In_ struct _IRP *Irp)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTKERNELAPI
VOID
NTAPI
PoSetDeviceBusyEx(
    _Inout_ PULONG IdlePointer)
{
    UNIMPLEMENTED;
    PoSetDeviceBusy(IdlePointer);
}

NTKRNLVISTAAPI
VOID
NTAPI
PoStartDeviceBusy(
    _Inout_ PULONG IdlePointer)
{
    UNIMPLEMENTED;
}

NTKERNELAPI
VOID
NTAPI
PoEndDeviceBusy(
    _Inout_ PULONG IdlePointer)
{
    UNIMPLEMENTED;
}


/*********************************************************************
 *                  strncpy_s   (NTDLL.@)
 */
errno_t __cdecl strncpy_s( char *dst, size_t len, const char *src, size_t count )
{
    size_t i, end;

    if (!count)
    {
        if (dst && len) *dst = 0;
        return 0;
    }
    if (!dst || !len) return EINVAL;
    if (!src)
    {
        *dst = 0;
        return EINVAL;
    }

    if (count != _TRUNCATE && count < len)
        end = count;
    else
        end = len - 1;

    for (i = 0; i < end; i++)
        if (!(dst[i] = src[i])) return 0;

    if (count == _TRUNCATE)
    {
        dst[i] = 0;
        return STRUNCATE;
    }
    if (end == count)
    {
        dst[i] = 0;
        return 0;
    }
    dst[0] = 0;
    return ERANGE;
}

/*
 * PROJECT:         ReactOS WDF Modern USB Stack
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         xHCI Driver Entrypoint
 * COPYRIGHT:       Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include "usbxhci.hpp"

EXTERN_C
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    return 0;
}

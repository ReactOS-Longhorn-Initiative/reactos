/*
 * PROJECT:         ReactOS WDF Modern USB Stack
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         USBHUB3 IOCTL
 * COPYRIGHT:       Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include "usbhub3.hpp"

#include  <debug.h>

/*
 * These IOCTLS seem to be major survivors from the transitions
 * Into WDF, while I've taking the structure from CDROM, and other sample WDF drivers
 * as reference. this file at it's heart is ioctl from USBHUB.sys
 */

NTSTATUS
FASTCALL
USBHub3IoctlGetHubCapabilities(/* TODO: */)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 1;
}

VOID
NTAPI
USBHub3IoctlGetPortStatus(/* TODO: */)
{
    UNIMPLEMENTED;
    __debugbreak();
}

/*
 * USBHub3IoctlGetNodeInformation
 * USBHub3IoctlGetNodeConnectionDriverKeyName
 * USBHub3IoctlGetNodeConnectionAttributes
 * USBHub3IoctlGetNodeName
 * USBHub3IoctlGetDescriptor
 * USBHub3IoctlCyclePort
 * USBHub3IoctlResetPort
 */

/* IOCTLs introduced with USBHUB3 -> Windows 8 RTM - NT6.2 RTM */
NTSTATUS
FASTCALL
USBHub3IoctlGetNodeConnectionInformationExV2()
{
    UNIMPLEMENTED;
    __debugbreak();
    return 1;
}

NTSTATUS
FASTCALL
USBHub3IoctlGetHubInformation (/* TODO: */)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 1;
}

NTSTATUS
FASTCALL
USBHub3IoctlGetPortConnectorProperties(/* TODO: */)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 1;
}

NTSTATUS
FASTCALL
USBHub3IoctlGetHubCapabilitiesEx(/* TODO: */)
{
    /* There's an Ex now..? */
    UNIMPLEMENTED;
    __debugbreak();
    return 1;
}
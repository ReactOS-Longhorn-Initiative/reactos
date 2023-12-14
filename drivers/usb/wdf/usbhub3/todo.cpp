/*
 * PROJECT:         ReactOS WDF Modern USB Stack
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         USBHUB3 Stubs
 * COPYRIGHT:       Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include "usbhub3.hpp"

#include  <debug.h>

VOID
NTAPI
USBHub3EvtDeviceUsageNotification(_In_ WDFDEVICE Device,
                                  _In_ WDF_SPECIAL_FILE_TYPE NotificationType,
                                  _In_ BOOLEAN IsInNotificationPath)
{
    UNIMPLEMENTED;
    __debugbreak();
}

NTSTATUS
NTAPI
USBHub3EvtDeviceReleaseHardware(_In_ WDFDEVICE Device,
                                _In_ WDFCMRESLIST ResourcesTranslated)
{
    UNIMPLEMENTED;
    __debugbreak();
    return 0;
}

NTSTATUS
NTAPI
USBHub3EvtDevicePrepareHardware(_In_ WDFDEVICE Device,
                                _In_ WDFCMRESLIST ResourcesRaw,
                                _In_ WDFCMRESLIST ResourcesTranslated)
{
    PAGED_CODE();
    DPRINT1("USBHub3EvtDevicePrepareHardware: Entry\n");
    /* An alternative to USBH_SymbolicLink could go here*/
    /* Let's obtain information about ports and map them here*/
    __debugbreak();
    return 0;
}

VOID
NTAPI
USBHub3EvtDeviceSurpriseRemoval(_In_ WDFDEVICE Device)
{
    UNIMPLEMENTED;
    __debugbreak();
}

NTSTATUS
NTAPI
USBHub3EvtDeviceD0Exit(_In_ WDFDEVICE Device,
                       _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    /* Unused - I doubt we need to touch this*/
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBHub3EvtDeviceD0Entry(_In_ WDFDEVICE Device,
                        _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    /* Unused - I doubt we need to touch this*/
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

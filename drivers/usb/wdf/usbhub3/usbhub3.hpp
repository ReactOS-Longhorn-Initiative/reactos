#pragma once

#include <ntddk.h>
#include <windef.h>
#include <hubbusif.h>
//#include <drivers/usbport/usbmport.h>

#include "ntddk.h"
#include "ntstrsafe.h"
#include "wdfcore.h"
#include "wdf.h"
#include <wdfldr.h>
#include "specstrings.h"

typedef struct _USBHUB3_DRIVER_EXTENSION {
    ULONG               Version;
    PDRIVER_OBJECT      DriverObject;
    ULONG               Flags;

} USBHUB3_DRIVER_EXTENSION, *PUSBHUB3_DRIVER_EXTENSION;

#define USBHUB_TAG 'UHUB'

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBHUB3_DRIVER_EXTENSION, DriverGetExtension)


/* PnP event callbacks*/
VOID
NTAPI
USBHub3EvtDeviceUsageNotification(_In_ WDFDEVICE Device,
                                  _In_ WDF_SPECIAL_FILE_TYPE NotificationType,
                                  _In_ BOOLEAN IsInNotificationPath);
NTSTATUS
NTAPI
USBHub3EvtDeviceReleaseHardware(_In_ WDFDEVICE Device,
                                _In_ WDFCMRESLIST ResourcesTranslated);

NTSTATUS
NTAPI
USBHub3EvtDevicePrepareHardware(_In_ WDFDEVICE Device,
                                _In_ WDFCMRESLIST ResourcesRaw,
                                _In_ WDFCMRESLIST ResourcesTranslated);

VOID
NTAPI
USBHub3EvtDeviceSurpriseRemoval(_In_ WDFDEVICE Device);

NTSTATUS
NTAPI
USBHub3EvtDeviceD0Exit(_In_ WDFDEVICE Device,
                       _In_ WDF_POWER_DEVICE_STATE TargetState);

NTSTATUS
NTAPI
USBHub3EvtDeviceD0Entry(_In_ WDFDEVICE Device,
                        _In_ WDF_POWER_DEVICE_STATE PreviousState);

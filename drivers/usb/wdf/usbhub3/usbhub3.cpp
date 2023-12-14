/*
 * PROJECT:         ReactOS WDF Modern USB Stack
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         usbhub3 entrypoint
 * COPYRIGHT:       Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

#include "usbhub3.hpp"

#include  <debug.h>

/*
 * The USB3 stack from Windows 8+ has a ton of drivers that need to be compatible
 * with the individual components. The good news is this resulted in a lot of the stack
 * being pretty well documented. I take liberties where I can but this should be reletively
 * close to what windows does.
 *
 * This is inspired by cdrom and the GPIO samples for reference of how WDF is supposed to work.
 */

typedef struct _USBHUB_DEVICE_EXTENSION {
    UINT32 Version;
} USBHUB_DEVICE_EXTENSION, *PUSBHUB_DEVICE_EXTENSION;

typedef struct _USBHUB_REQUEST_CONTEXT {
    PUSBHUB_DEVICE_EXTENSION     DeviceExtension;
} USBHUB_REQUEST_CONTEXT, *PUSBHUB_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USBHUB_REQUEST_CONTEXT, RequestGetContext)

VOID
NTAPI
USBHub3Cleanup(_In_ WDFOBJECT Object)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
USBHub3EvtDeviceAdd(_In_ WDFDRIVER Driver,
                    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status;
    WDFDEVICE                       device = NULL;
    WDF_OBJECT_ATTRIBUTES           attributes;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    PAGED_CODE();

    /* PNPOWER Evt Callbacks */
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    /* Prep Exit lower power mode */
    pnpPowerCallbacks.EvtDeviceD0Entry = USBHub3EvtDeviceD0Entry;
    /* Prep Entering lower power mode */
    pnpPowerCallbacks.EvtDeviceD0Exit = USBHub3EvtDeviceD0Exit;
    pnpPowerCallbacks.EvtDeviceSurpriseRemoval = USBHub3EvtDeviceSurpriseRemoval;
    pnpPowerCallbacks.EvtDevicePrepareHardware = USBHub3EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = USBHub3EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceUsageNotification = USBHub3EvtDeviceUsageNotification;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    /*
     * Windows seems to create a WDF_POWER_POLICY_EVENT_CALLBACKS list somewhere in this function
     * Doesn't seem nessesary for compatibility and we don't even support this.
     *
     * TODO: PowerPolicy support
     */
    //Reuse this structure.
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes,
                                            USBHUB_REQUEST_CONTEXT);
    attributes.EvtCleanupCallback = USBHub3Cleanup;

    WdfDeviceInitSetRequestAttributes(DeviceInit,
                                        &attributes);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status))
    {
        DPRINT1("DriverEvtDeviceAdd: Can not create a new device, status: 0x%X\n",
                    status);
    }
    return status;
}

EXTERN_C
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS                status;
    WDF_DRIVER_CONFIG       config;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDRIVER               driverObject = NULL;

    DPRINT1("ReactOS USBHUB3 Entry\n");
    PAGED_CODE();

    // Register DeviceAdd and DriverEvtCleanup callback.
    // WPP_CLEANUP will be called in DriverEvtCleanup
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, USBHUB3_DRIVER_EXTENSION);
    attributes.EvtCleanupCallback = &USBHub3Cleanup;
    config.DriverPoolTag = USBHUB_TAG;
    WDF_DRIVER_CONFIG_INIT(&config, &USBHub3EvtDeviceAdd);

    /*
     * While parts of the USB stack is the "undocumented extension"
     * USBHUB3 is a standard WDF Driver
     */
    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             &driverObject);
    if (status != STATUS_SUCCESS)
    {
        DPRINT1("WdfDriverCreate: exited with Status %u", status);
    }

    attributes.ParentObject = driverObject;
    return status;
}

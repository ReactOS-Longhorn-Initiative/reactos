#include "precomp.h"
#include "acpi_new.h"

#include <poclass.h>

typedef struct _ACPI_NEW_NOTIFY_ENTRY
{
    LIST_ENTRY Link;
    BOOLEAN V2;
    union
    {
        PDEVICE_NOTIFY_CALLBACK V1;
        PDEVICE_NOTIFY_CALLBACK2 V2Cb;
    } Callback;
    PVOID NotificationContext;
} ACPI_NEW_NOTIFY_ENTRY, *PACPI_NEW_NOTIFY_ENTRY;

static
NTSTATUS
AcpiNewHandlePdoPnp(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PACPI_NEW_PDO_EXTENSION pdoExt = (PACPI_NEW_PDO_EXTENSION)DeviceObject->DeviceExtension;
    NTSTATUS status = STATUS_NOT_SUPPORTED;

    switch (irpSp->MinorFunction)
    {
    case IRP_MN_START_DEVICE:
    {
        NTSTATUS st;

        if (pdoExt->HardwareIds)
        {
            if (!pdoExt->SysButtonInterfaceEnabled &&
                (wcsstr(pdoExt->HardwareIds, L"PNP0C0C") != NULL ||
                 wcsstr(pdoExt->HardwareIds, L"PNP0C0E") != NULL ||
                 wcsstr(pdoExt->HardwareIds, L"ACPI_FPB") != NULL ||
                 wcsstr(pdoExt->HardwareIds, L"ACPI_FSB") != NULL))
            {
                st = IoRegisterDeviceInterface(DeviceObject, &GUID_DEVICE_SYS_BUTTON, NULL, &pdoExt->SysButtonInterface);
                if (NT_SUCCESS(st))
                {
                    (void)IoSetDeviceInterfaceState(&pdoExt->SysButtonInterface, TRUE);
                    pdoExt->SysButtonInterfaceEnabled = TRUE;
                }
            }

            if (!pdoExt->LidInterfaceEnabled &&
                (wcsstr(pdoExt->HardwareIds, L"PNP0C0D") != NULL))
            {
                st = IoRegisterDeviceInterface(DeviceObject, &GUID_DEVICE_LID, NULL, &pdoExt->LidInterface);
                if (NT_SUCCESS(st))
                {
                    (void)IoSetDeviceInterfaceState(&pdoExt->LidInterface, TRUE);
                    pdoExt->LidInterfaceEnabled = TRUE;
                }
            }

            if (!pdoExt->ThermalZoneInterfaceEnabled &&
                (wcsstr(pdoExt->HardwareIds, L"ThermalZone") != NULL))
            {
                st = IoRegisterDeviceInterface(DeviceObject, &GUID_DEVICE_THERMAL_ZONE, NULL, &pdoExt->ThermalZoneInterface);
                if (NT_SUCCESS(st))
                {
                    (void)IoSetDeviceInterfaceState(&pdoExt->ThermalZoneInterface, TRUE);
                    pdoExt->ThermalZoneInterfaceEnabled = TRUE;
                }
            }

            if (!pdoExt->FanInterfaceEnabled &&
                (wcsstr(pdoExt->HardwareIds, L"PNP0C0B") != NULL))
            {
                st = IoRegisterDeviceInterface(DeviceObject, &GUID_DEVICE_FAN, NULL, &pdoExt->FanInterface);
                if (NT_SUCCESS(st))
                {
                    (void)IoSetDeviceInterfaceState(&pdoExt->FanInterface, TRUE);
                    pdoExt->FanInterfaceEnabled = TRUE;
                }
            }

            if (!pdoExt->ProcessorInterfaceEnabled &&
                (wcsstr(pdoExt->HardwareIds, L"Processor") != NULL))
            {
                st = IoRegisterDeviceInterface(DeviceObject, &GUID_DEVICE_PROCESSOR, NULL, &pdoExt->ProcessorInterface);
                if (NT_SUCCESS(st))
                {
                    (void)IoSetDeviceInterfaceState(&pdoExt->ProcessorInterface, TRUE);
                    pdoExt->ProcessorInterfaceEnabled = TRUE;
                }
            }
        }

        status = STATUS_SUCCESS;
        break;
    }

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_STOP_DEVICE:
    case IRP_MN_SURPRISE_REMOVAL:
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (irpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
        {
            PDEVICE_RELATIONS rel;
            rel = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(PagedPool,
                                                          sizeof(DEVICE_RELATIONS),
                                                          'rAcu');
            if (!rel)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            rel->Count = 1;
            ObReferenceObject(DeviceObject);
            rel->Objects[0] = DeviceObject;
            Irp->IoStatus.Information = (ULONG_PTR)rel;
            status = STATUS_SUCCESS;
        }
        break;

    case IRP_MN_QUERY_RESOURCES:
        status = AcpiNewPdoQueryResources(pdoExt, Irp);
        break;

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        status = AcpiNewPdoQueryResourceRequirements(pdoExt, Irp);
        break;

    case IRP_MN_QUERY_INTERFACE:
        status = AcpiNewPdoQueryInterface(pdoExt, Irp);
        break;

    case IRP_MN_QUERY_DEVICE_TEXT:
        if (irpSp->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription)
        {
            const WCHAR *text = L"ACPI device";
            SIZE_T chars;
            PWSTR buf;

            if (pdoExt->HardwareIds)
            {
                if (wcsstr(pdoExt->HardwareIds, L"PNP0A03") != NULL ||
                    wcsstr(pdoExt->HardwareIds, L"PNP0A08") != NULL)
                    text = L"PCI Root Bridge";
                else if (wcsstr(pdoExt->HardwareIds, L"PNP0303") != NULL)
                    text = L"Keyboard";
                else if (wcsstr(pdoExt->HardwareIds, L"PNP0F03") != NULL)
                    text = L"Mouse";
            }

            chars = wcslen(text);
            buf = (PWSTR)ExAllocatePoolWithTag(PagedPool, (chars + 1) * sizeof(WCHAR), 'tAcu');
            if (!buf)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            RtlCopyMemory(buf, text, (chars + 1) * sizeof(WCHAR));
            Irp->IoStatus.Information = (ULONG_PTR)buf;
            status = STATUS_SUCCESS;
        }
        break;

    case IRP_MN_QUERY_BUS_INFORMATION:
    {
        PPNP_BUS_INFORMATION busInfo;
        busInfo = (PPNP_BUS_INFORMATION)ExAllocatePoolWithTag(PagedPool,
                                                             sizeof(PNP_BUS_INFORMATION),
                                                             'IpcA');
        if (!busInfo)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        busInfo->BusTypeGuid = GUID_ACPI_INTERFACE_STANDARD;
        busInfo->LegacyBusType = InternalPowerBus;
        busInfo->BusNumber = 0;
        Irp->IoStatus.Information = (ULONG_PTR)busInfo;
        status = STATUS_SUCCESS;
        break;
    }

    case IRP_MN_QUERY_ID:
    {
        PWSTR result = NULL;
        status = STATUS_SUCCESS;
        switch (irpSp->Parameters.QueryId.IdType)
        {
        case BusQueryDeviceID:
            if (pdoExt->DeviceId)
                result = AcpiNewDupUnicodeString(pdoExt->DeviceId, wcslen(pdoExt->DeviceId));
            break;

        case BusQueryHardwareIDs:
            if (pdoExt->HardwareIds)
                result = AcpiNewDupMultiSz(pdoExt->HardwareIds);
            break;

        case BusQueryCompatibleIDs:
            if (pdoExt->CompatibleIds)
                result = AcpiNewDupMultiSz(pdoExt->CompatibleIds);
            break;

        case BusQueryInstanceID:
            if (pdoExt->InstanceId)
                result = AcpiNewDupUnicodeString(pdoExt->InstanceId, wcslen(pdoExt->InstanceId));
            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;
        }

        if (status == STATUS_SUCCESS)
        {
            if (!result)
                status = STATUS_INSUFFICIENT_RESOURCES;
            else
                Irp->IoStatus.Information = (ULONG_PTR)result;
        }
        break;
    }

    case IRP_MN_QUERY_CAPABILITIES:
        if (irpSp->Parameters.DeviceCapabilities.Capabilities)
        {
            PDEVICE_CAPABILITIES caps = irpSp->Parameters.DeviceCapabilities.Capabilities;
            caps->SurpriseRemovalOK = FALSE;
            caps->Removable = FALSE;
            caps->EjectSupported = FALSE;
            caps->Address = (ULONG)-1;
            caps->UINumber = (ULONG)-1;
        }
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        Irp->IoStatus.Information = 0;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_REMOVE_DEVICE:
        status = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        PLIST_ENTRY entry;

        if (pdoExt->SysButtonInterfaceEnabled)
        {
            (void)IoSetDeviceInterfaceState(&pdoExt->SysButtonInterface, FALSE);
            RtlFreeUnicodeString(&pdoExt->SysButtonInterface);
            pdoExt->SysButtonInterfaceEnabled = FALSE;
            RtlZeroMemory(&pdoExt->SysButtonInterface, sizeof(pdoExt->SysButtonInterface));
        }

        if (pdoExt->LidInterfaceEnabled)
        {
            (void)IoSetDeviceInterfaceState(&pdoExt->LidInterface, FALSE);
            RtlFreeUnicodeString(&pdoExt->LidInterface);
            pdoExt->LidInterfaceEnabled = FALSE;
            RtlZeroMemory(&pdoExt->LidInterface, sizeof(pdoExt->LidInterface));
        }

        if (pdoExt->ThermalZoneInterfaceEnabled)
        {
            (void)IoSetDeviceInterfaceState(&pdoExt->ThermalZoneInterface, FALSE);
            RtlFreeUnicodeString(&pdoExt->ThermalZoneInterface);
            pdoExt->ThermalZoneInterfaceEnabled = FALSE;
            RtlZeroMemory(&pdoExt->ThermalZoneInterface, sizeof(pdoExt->ThermalZoneInterface));
        }

        if (pdoExt->FanInterfaceEnabled)
        {
            (void)IoSetDeviceInterfaceState(&pdoExt->FanInterface, FALSE);
            RtlFreeUnicodeString(&pdoExt->FanInterface);
            pdoExt->FanInterfaceEnabled = FALSE;
            RtlZeroMemory(&pdoExt->FanInterface, sizeof(pdoExt->FanInterface));
        }

        if (pdoExt->ProcessorInterfaceEnabled)
        {
            (void)IoSetDeviceInterfaceState(&pdoExt->ProcessorInterface, FALSE);
            RtlFreeUnicodeString(&pdoExt->ProcessorInterface);
            pdoExt->ProcessorInterfaceEnabled = FALSE;
            RtlZeroMemory(&pdoExt->ProcessorInterface, sizeof(pdoExt->ProcessorInterface));
        }

        ExAcquireFastMutex(&pdoExt->NotifyLock);
        while (!IsListEmpty(&pdoExt->NotifyList))
        {
            PACPI_NEW_NOTIFY_ENTRY notifyEntry;
            entry = RemoveHeadList(&pdoExt->NotifyList);
            notifyEntry = CONTAINING_RECORD(entry, ACPI_NEW_NOTIFY_ENTRY, Link);
            ExFreePoolWithTag(notifyEntry, 'nAcu');
        }
        ExReleaseFastMutex(&pdoExt->NotifyLock);

        if (pdoExt->DeviceId) ExFreePoolWithTag(pdoExt->DeviceId, 'iAcu');
        if (pdoExt->HardwareIds) ExFreePoolWithTag(pdoExt->HardwareIds, 'hAcu');
        if (pdoExt->CompatibleIds) ExFreePoolWithTag(pdoExt->CompatibleIds, 'cAcu');
        if (pdoExt->InstanceId) ExFreePoolWithTag(pdoExt->InstanceId, 'dAcu');
        IoDeleteDevice(DeviceObject);
    }

    return status;
}

static
NTSTATUS
AcpiNewHandleFdoPnp(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    PACPI_NEW_FDO_EXTENSION fdoExt = (PACPI_NEW_FDO_EXTENSION)DeviceObject->DeviceExtension;
    NTSTATUS status;

    switch (irpSp->MinorFunction)
    {
    case IRP_MN_START_DEVICE:
        status = AcpiNewForwardIrpSynchronously(fdoExt, Irp);
        if (!NT_SUCCESS(status))
        {
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }

        status = AcpiNewStartUacpiAndEnumerate(fdoExt);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        if (irpSp->Parameters.QueryDeviceRelations.Type == BusRelations)
        {
            ULONG count = 0;
            PLIST_ENTRY entry;
            PDEVICE_RELATIONS rel;

            if (InterlockedExchange(&fdoExt->EnumerationDirty, 0) != 0)
                AcpiNewRefreshEnumeration(fdoExt);

            ExAcquireFastMutex(&fdoExt->Mutex);
            for (entry = fdoExt->PdoList.Flink; entry != &fdoExt->PdoList; entry = entry->Flink)
            {
                PACPI_NEW_PDO_EXTENSION pdoExt = CONTAINING_RECORD(entry, ACPI_NEW_PDO_EXTENSION, Link);
                if (pdoExt->Present)
                    count++;
            }

            rel = (PDEVICE_RELATIONS)ExAllocatePoolWithTag(PagedPool,
                                                          sizeof(DEVICE_RELATIONS) + sizeof(PDEVICE_OBJECT) * (count ? (count - 1) : 0),
                                                          'rAcu');
            if (!rel)
            {
                ExReleaseFastMutex(&fdoExt->Mutex);
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            rel->Count = 0;
            for (entry = fdoExt->PdoList.Flink; entry != &fdoExt->PdoList; entry = entry->Flink)
            {
                PACPI_NEW_PDO_EXTENSION pdoExt = CONTAINING_RECORD(entry, ACPI_NEW_PDO_EXTENSION, Link);
                if (!pdoExt->Present)
                    continue;
                ObReferenceObject(pdoExt->Common.Self);
                rel->Objects[rel->Count++] = pdoExt->Common.Self;
            }
            ExReleaseFastMutex(&fdoExt->Mutex);

            Irp->IoStatus.Information = (ULONG_PTR)rel;
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }

        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(fdoExt->LowerDevice, Irp);

    case IRP_MN_REMOVE_DEVICE:
    {
        PLIST_ENTRY entry, next;
        fdoExt->Removed = TRUE;

        (void)uacpi_uninstall_notify_handler(
            uacpi_namespace_root(),
            AcpiNewNotifyHandler
        );

        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(fdoExt->LowerDevice, Irp);

        ExAcquireFastMutex(&fdoExt->Mutex);
        entry = fdoExt->PdoList.Flink;
        while (entry != &fdoExt->PdoList)
        {
            PACPI_NEW_PDO_EXTENSION pdoExt = CONTAINING_RECORD(entry, ACPI_NEW_PDO_EXTENSION, Link);
            next = entry->Flink;
            RemoveEntryList(entry);
            pdoExt->Present = FALSE;
            entry = next;
        }
        ExReleaseFastMutex(&fdoExt->Mutex);

        IoDetachDevice(fdoExt->LowerDevice);
        IoDeleteDevice(DeviceObject);
        return status;
    }

    default:
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(fdoExt->LowerDevice, Irp);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
NTAPI
AcpiNewDispatchPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    if (AcpiNewIsPdo(DeviceObject))
        return AcpiNewHandlePdoPnp(DeviceObject, Irp);
    if (AcpiNewIsFdo(DeviceObject))
        return AcpiNewHandleFdoPnp(DeviceObject, Irp);

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_INVALID_DEVICE_REQUEST;
}

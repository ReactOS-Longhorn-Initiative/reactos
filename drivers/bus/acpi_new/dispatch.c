#include "precomp.h"
#include "acpi_new.h"
#include <uacpi/status.h>

static NTSTATUS
AcpiNewMapSystemPowerToUacpi(
    _In_ SYSTEM_POWER_STATE SystemState,
    _Out_ uacpi_sleep_state *OutState)
{
    switch (SystemState)
    {
        case PowerSystemSleeping1:
            *OutState = UACPI_SLEEP_STATE_S1;
            return STATUS_SUCCESS;
        case PowerSystemSleeping2:
            *OutState = UACPI_SLEEP_STATE_S2;
            return STATUS_SUCCESS;
        case PowerSystemSleeping3:
            *OutState = UACPI_SLEEP_STATE_S3;
            return STATUS_SUCCESS;
        case PowerSystemHibernate:
            *OutState = UACPI_SLEEP_STATE_S4;
            return STATUS_SUCCESS;
        default:
            return STATUS_NOT_SUPPORTED;
    }
}

static NTSTATUS
NTAPI
AcpiNewSystemSleepDownCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PACPI_NEW_FDO_EXTENSION FdoExt = Context;
    uacpi_status Ust;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(DeviceObject);

    PoStartNextPowerIrp(Irp);

    if (!NT_SUCCESS(Irp->IoStatus.Status) || FdoExt->PendingUacpiSleep == UACPI_SLEEP_STATE_S0)
    {
        FdoExt->PendingUacpiSleep = UACPI_SLEEP_STATE_S0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    Ust = uacpi_enter_sleep_state(FdoExt->PendingUacpiSleep);
    KeLowerIrql(OldIrql);

    FdoExt->PendingUacpiSleep = UACPI_SLEEP_STATE_S0;

    if (uacpi_unlikely_error(Ust))
    {
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
AcpiNewDispatchPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (AcpiNewIsFdo(DeviceObject))
    {
        PACPI_NEW_FDO_EXTENSION FdoExt = (PACPI_NEW_FDO_EXTENSION)DeviceObject->DeviceExtension;

        if (IrpSp->MinorFunction == IRP_MN_SET_POWER &&
            IrpSp->Parameters.Power.Type == SystemPowerState)
        {
            SYSTEM_POWER_STATE SysState = IrpSp->Parameters.Power.State.SystemState;
            uacpi_sleep_state UacpiState;
            NTSTATUS MapStatus;
            uacpi_status Ust;

            if (SysState == PowerSystemWorking)
            {
                if (FdoExt->PendingUacpiSleep != UACPI_SLEEP_STATE_S0)
                {
                    KIRQL OldIrql;

                    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
                    Ust = uacpi_prepare_for_wake_from_sleep_state(FdoExt->PendingUacpiSleep);
                    KeLowerIrql(OldIrql);

                    if (uacpi_unlikely_error(Ust))
                    {
                        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                        PoStartNextPowerIrp(Irp);
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return STATUS_UNSUCCESSFUL;
                    }

                    Ust = uacpi_wake_from_sleep_state(FdoExt->PendingUacpiSleep);
                    FdoExt->PendingUacpiSleep = UACPI_SLEEP_STATE_S0;

                    if (uacpi_unlikely_error(Ust))
                    {
                        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                        PoStartNextPowerIrp(Irp);
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return STATUS_UNSUCCESSFUL;
                    }
                }

                PoStartNextPowerIrp(Irp);
                IoSkipCurrentIrpStackLocation(Irp);
                return PoCallDriver(FdoExt->LowerDevice, Irp);
            }

            MapStatus = AcpiNewMapSystemPowerToUacpi(SysState, &UacpiState);
            if (!NT_SUCCESS(MapStatus))
            {
                PoStartNextPowerIrp(Irp);
                IoSkipCurrentIrpStackLocation(Irp);
                return PoCallDriver(FdoExt->LowerDevice, Irp);
            }

            Ust = uacpi_prepare_for_sleep_state(UacpiState);
            if (uacpi_unlikely_error(Ust))
            {
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                PoStartNextPowerIrp(Irp);
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_UNSUCCESSFUL;
            }

            FdoExt->PendingUacpiSleep = UacpiState;

            PoStartNextPowerIrp(Irp);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   AcpiNewSystemSleepDownCompletion,
                                   FdoExt,
                                   TRUE,
                                   TRUE,
                                   TRUE);
            return PoCallDriver(FdoExt->LowerDevice, Irp);
        }

        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(FdoExt->LowerDevice, Irp);
    }

    PoStartNextPowerIrp(Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
AcpiNewDispatchCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
AcpiNewDispatchDefault(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    if (AcpiNewIsFdo(DeviceObject))
    {
        PACPI_NEW_FDO_EXTENSION FdoExt = (PACPI_NEW_FDO_EXTENSION)DeviceObject->DeviceExtension;

        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(FdoExt->LowerDevice, Irp);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

#include <rxgkrnl.h>

#include <debug.h>

PRXGK_PRIVATE_EXTENSION RxgkDriverExtension;

/**
 * @brief The first real transaction between WDDM Miniport and Dxgkrnl, this internal routine
 * gets called by Displib and passes the callback list into a internal struct
 *
 * @param DriverObject DriverObject of the miniport driver itself
 *
 * @param SourceString The registry path provided by Displib
 *
 * @param DriverInitData The callback list provided by a WDDM miniport driver via displib
 *
 * @return NTSTATUS Standard NT return value
 */
NTSTATUS
NTAPI
RxgkPortInitializeMiniport(_In_ PDRIVER_OBJECT DriverObject,
                           _In_ PUNICODE_STRING SourceString,
                           _In_ PDRIVER_INITIALIZATION_DATA DriverInitData)
{
    NTSTATUS Status;

    PAGED_CODE();
    if (!DriverObject || !SourceString || !DriverInitData)
        return STATUS_INVALID_PARAMETER;

    /* let's make sure we can allocate the private extension */
    Status = IoAllocateDriverObjectExtension(DriverObject, DriverObject, sizeof(RXGK_PRIVATE_EXTENSION), (PVOID*)&RxgkDriverExtension);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("DxgkPortInitializeMiniport: Couldn't allocate object extension status: 0x%X", Status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RxgkDriverExtension->Version = DriverInitData->Version;
    RxgkDriverExtension->DxgkDdiAddDevice = DriverInitData->DxgkDdiAddDevice;
    RxgkDriverExtension->DxgkDdiStartDevice = DriverInitData->DxgkDdiStartDevice;
    RxgkDriverExtension->DxgkDdiStopDevice = DriverInitData->DxgkDdiStopDevice;
    RxgkDriverExtension->DxgkDdiRemoveDevice = DriverInitData->DxgkDdiRemoveDevice;
    RxgkDriverExtension->DxgkDdiDispatchIoRequest = DriverInitData->DxgkDdiDispatchIoRequest;
    RxgkDriverExtension->DxgkDdiInterruptRoutine = DriverInitData->DxgkDdiInterruptRoutine;
    RxgkDriverExtension->DxgkDdiDpcRoutine = DriverInitData->DxgkDdiDpcRoutine;
    RxgkDriverExtension->DxgkDdiRecommendFunctionalVidPn = DriverInitData->DxgkDdiRecommendFunctionalVidPn;
    RxgkDriverExtension->DxgkDdiEnumVidPnCofuncModality = DriverInitData->DxgkDdiEnumVidPnCofuncModality;
    RxgkDriverExtension->DxgkDdiQueryAdapterInfo = DriverInitData->DxgkDdiQueryAdapterInfo;
    DPRINT1("RDDM: WDDM Miniport driver reports a version of %X\n", RxgkDriverExtension->Version);
    PDRIVER_EXTENSION DriverExtend;

    /* Fill out the internal structure - WIP */
    RxgkDriverExtension->MiniportDriverObject = DriverObject;
    RxgkDriverExtension->InternalDeviceNumber = 0;

    /* Fill out the public dispatch routines */
    DriverExtend = DriverObject->DriverExtension;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)RxgkPortDispatchCreateDevice;
    DriverObject->MajorFunction[IRP_MJ_PNP] = (PDRIVER_DISPATCH)RxgkPortDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = (PDRIVER_DISPATCH)RxgkPortDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)RxgkPortDispatchIoctl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = (PDRIVER_DISPATCH)RxgkPortDispatchInternalIoctl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = (PDRIVER_DISPATCH)RxgkPortDispatchSystemControl;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)RxgkPortDispatchCloseDevice;

    DriverExtend->AddDevice = RxgkPortAddDevice;
    DriverObject->DriverUnload = (PDRIVER_UNLOAD)RxgkPortDriverUnload;
    DPRINT("RxgkPortInitializeMiniport: Finished\n");
    return Status;
}

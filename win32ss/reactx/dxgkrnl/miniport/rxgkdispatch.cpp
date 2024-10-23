#include <rxgkrnl.h>

#include <debug.h>


extern PRXGK_PRIVATE_EXTENSION RxgkDriverExtension;
extern DXGKRNL_INTERFACE DxgkrnlInterface;


BOOLEAN NTAPI
IntVideoPortInterruptRoutine(
   IN struct _KINTERRUPT *Interrupt,
   IN PVOID ServiceContext)
{
    DPRINT1("RxgkIntVideoPortInterruptRoutine: Entry");
    return RxgkDriverExtension->DxgkDdiInterruptRoutine(RxgkDriverExtension->MiniportContext, 0);
}


BOOLEAN NTAPI
IntVideoPortSetupInterrupt()
{
    if (RxgkDriverExtension->AdapterInterfaceType == PCIBus)
        RxgkDriverExtension->InterruptMode = LevelSensitive;
    else
        RxgkDriverExtension->InterruptMode = Latched;

   NTSTATUS Status;
   if ((RxgkDriverExtension->BusInterruptLevel != 0 ||
       RxgkDriverExtension->BusInterruptVector != 0))
   {
      ULONG InterruptVector;
      KIRQL Irql;
      KAFFINITY Affinity;

      InterruptVector = HalGetInterruptVector(
         RxgkDriverExtension->AdapterInterfaceType,
         RxgkDriverExtension->SystemIoBusNumber,
         RxgkDriverExtension->BusInterruptLevel,
         RxgkDriverExtension->BusInterruptVector,
         &Irql,
         &Affinity);

      if (InterruptVector == 0)
      {
         DPRINT1("HalGetInterruptVector failed\n");
         return FALSE;
      }

      KeInitializeSpinLock(&RxgkDriverExtension->InterruptSpinLock);
      Status = IoConnectInterrupt(
         &RxgkDriverExtension->InterruptObject,
         IntVideoPortInterruptRoutine,
         RxgkDriverExtension,
         &RxgkDriverExtension->InterruptSpinLock,
         InterruptVector,
         Irql,
         Irql,
         RxgkDriverExtension->InterruptMode,
         RxgkDriverExtension->InterruptShared,
         Affinity,
         FALSE);

      if (!NT_SUCCESS(Status))
      {
         DPRINT1("IoConnectInterrupt failed with status 0x%08x\n", Status);
         return FALSE;
      }
   }

   return TRUE;
}

VOID
NTAPI
RxgkSetupInterrupts()
{
    CM_FULL_RESOURCE_DESCRIPTOR *FullList;
    CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;

    PCM_RESOURCE_LIST ResourceList;
    DxgkrnlSetupResourceList(&ResourceList);
    FullList = ResourceList->List;
    for (Descriptor = FullList->PartialResourceList.PartialDescriptors;
         Descriptor < FullList->PartialResourceList.PartialDescriptors + FullList->PartialResourceList.Count;
         Descriptor++)
    {
        if (Descriptor->Type == CmResourceTypeInterrupt)
        {
            RxgkDriverExtension->BusInterruptLevel = Descriptor->u.Interrupt.Level;
            RxgkDriverExtension->BusInterruptVector = Descriptor->u.Interrupt.Vector;
            if (Descriptor->ShareDisposition == CmResourceShareShared)
                RxgkDriverExtension->InterruptShared = TRUE;
            else
                RxgkDriverExtension->InterruptShared = FALSE;

            DPRINT1("InterruptLevel: %X, InterruptVector %X\n", RxgkDriverExtension->BusInterruptLevel, RxgkDriverExtension->BusInterruptVector );
        }
    }

    IntVideoPortSetupInterrupt();
}

NTSTATUS
NTAPI
RxgkpInitializePCI()
{
    NTSTATUS Status;
    GUID Bus = {0x496b8280, 0x6f25, 0x11d0, 0xbe, 0xaf, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f};

    Status = RxgkpQueryInterface(RxgkDriverExtension,
                                   &Bus,
                                   (PVOID)&RxgkDriverExtension->BusInterface,
                                   sizeof(BUS_INTERFACE_STANDARD));
    if (Status == STATUS_SUCCESS)
    {
        DPRINT1("DxgkpQueryInterface: Device has success context:0x%X\n", RxgkDriverExtension->BusInterface.Context);
    }
    else{
        DPRINT1("DxgkPortStartAdapter: Failed with Status %d\n", Status);
        __debugbreak();
        return Status;
    }

    return Status;
}

NTSTATUS
NTAPI
RxgkStartAdapter()
{
    NTSTATUS Status;
    ULONG AdapterNumberOfVideoPresentSources;
    ULONG AdapterNumberOfChildren;

    /* Initialize AdapterInterface Communication*/
    DPRINT1("RxgkStartAdapter: initializing bus communication.\n");
    if (RxgkDriverExtension->AdapterInterfaceType == PCIBus)
    {
        Status = RxgkpInitializePCI();
        if (Status != STATUS_SUCCESS)
            return Status;
    }

    /* Acquire StartInfo information - ?*/
    DXGK_START_INFO     DxgkStartInfo = {0};
    DxgkStartInfo.RequiredDmaQueueEntry = 1;
    DxgkStartInfo.AdapterGuid = {0};
    DxgkStartInfo.AdapterGuid.Data1 = 1;
    /* Dxgkrnl Callbacks */
    /* Interrupt routine information*/
    RxgkSetupInterrupts();
    /* Calling start Adapter */
    DPRINT1("RxgkStartAdapter: Calling Miniport StartAdapter\n");
    Status = RxgkDriverExtension->DxgkDdiStartDevice(RxgkDriverExtension->MiniportContext,
                                                     &DxgkStartInfo,
                                                     &DxgkrnlInterface,
                                                     &AdapterNumberOfVideoPresentSources,
                                                     &AdapterNumberOfChildren);
    DPRINT1("RxgkDriverExtension->DxgkDdiStartDevice: returned with Status %X\n", Status);

    return Status;
}

VOID
NTAPI
IntVideoPortDeferredRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
{
    DPRINT1("IntVideoPortDeferredRoutine: Dxgkrnl entry\n");
    PVOID HwDeviceExtension = &((PRXGK_PRIVATE_EXTENSION)DeferredContext)->MiniportContext;
        RxgkDriverExtension->DxgkDdiDpcRoutine(HwDeviceExtension);
}

/**
 * @brief Intercepts and calls the AddDevice Miniport call back
 *
 * @param DriverObject - Pointer to DRIVER_OBJECT structure
 *
 * @param PhysicalDeviceObject - Pointer to Miniport DEVICE_OBJECT structure
 *
 * @return NTSTATUS
 */
NTSTATUS
NTAPI
RxgkPortAddDevice(_In_    DRIVER_OBJECT *DriverObject,
                  _Inout_ DEVICE_OBJECT *PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT Fdo;
    WCHAR DeviceBuffer[20];
    UNICODE_STRING DeviceName;
    PCI_SLOT_NUMBER SlotNumber;
    ULONG PciSlotNumber;
    ULONG Size;

    ULONG_PTR Context = 0;

    PAGED_CODE();

    /* MS does a whole bunch of bullcrap here so we will try to track it */
    if (!DriverObject || !PhysicalDeviceObject)
    {
        DPRINT1("RxgkPortAddDevice: wrong parameters");
        return STATUS_INVALID_PARAMETER;
    }

    /* Call the miniport Routine */
    Status = RxgkDriverExtension->DxgkDdiAddDevice(PhysicalDeviceObject, (PVOID*)&Context);
    if(Status != STATUS_SUCCESS)
    {
        DPRINT1("DxgkPortAddDevice: AddDevice Miniport call failed with status %X\n", Status);
    }
    else{
        DPRINT1("DxgkPortAddDevice: AddDevice Miniport call has continued with success\n");
    }

    /* Create a Video Device */
    swprintf(DeviceBuffer, L"\\Device\\Video%lu", 0);
    RtlInitUnicodeString(&DeviceName, DeviceBuffer);
    RxgkDriverExtension->MiniportContext = (PVOID)Context;
    Status = IoCreateDevice(DriverObject,
                            0,
                            &DeviceName,
                            FILE_DEVICE_VIDEO,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice() failed with status 0x%08x\n", Status);
        return Status;
    }

    RxgkDriverExtension->MiniportFdo = Fdo;
    RxgkDriverExtension->MiniportPdo = PhysicalDeviceObject;

    /* Figure our bus*/
    Size = sizeof(ULONG);
    IoGetDeviceProperty(RxgkDriverExtension->MiniportPdo,
                                 DevicePropertyBusNumber,
                                 Size,
                                 &RxgkDriverExtension->SystemIoBusNumber,
                                 &Size);
    Size = sizeof(ULONG);
    IoGetDeviceProperty(RxgkDriverExtension->MiniportPdo,
                        DevicePropertyLegacyBusType,
                        Size,
                        &RxgkDriverExtension->AdapterInterfaceType,
                        &Size);
    DPRINT1("AdapterInterfaceType :%d\n", RxgkDriverExtension->AdapterInterfaceType);


    /* Figure out our device */
    Size = sizeof(ULONG);
    IoGetDeviceProperty(RxgkDriverExtension->MiniportPdo,
                        DevicePropertyAddress,
                        Size,
                        &PciSlotNumber,
                        &Size);
    SlotNumber.u.AsULONG = 0;
    SlotNumber.u.bits.DeviceNumber = (PciSlotNumber >> 16) & 0xFFFF;
    SlotNumber.u.bits.FunctionNumber = PciSlotNumber & 0xFFFF;
    RxgkDriverExtension->SystemIoSlotNumber = SlotNumber.u.AsULONG;

    DPRINT1("Device Number: %d\n",  SlotNumber.u.bits.DeviceNumber);
    DPRINT1("FunctionNumber: %d\n", SlotNumber.u.bits.FunctionNumber);
    DPRINT1("Create IDs success\n");

    KeInitializeDpc((PRKDPC)&RxgkDriverExtension->DpcObject,
                    IntVideoPortDeferredRoutine,
                    RxgkDriverExtension);

    /* Remove the initializing flag */
    (DriverObject->DeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;
    RxgkDriverExtension->NextDeviceObject = IoAttachDeviceToDeviceStack(
                                                DriverObject->DeviceObject,
                                                PhysicalDeviceObject);

    /* match the path videoprt uses. */
    DPRINT("RxgkPortAddDevice: Driver attach success\n");
    Status = IntCreateNewRegistryPath(RxgkDriverExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IntCreateNewRegistryPath() failed with status 0x%08x\n", Status);
        return Status;
    }

    /* Set up the VIDEO/DEVICEMAP registry keys */
    DPRINT("RxgkPortAddDevice: registry setup path success\n");
    Status = IntVideoPortAddDeviceMapLink(RxgkDriverExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IntVideoPortAddDeviceMapLink() failed with status 0x%08x\n", Status);
        return Status;
    }

    DPRINT1("RxgkPortAddDevice: Device Creation sucessful \n");
    return Status;
}

/*
 * @ UNIMPLEMENTED
 */
NTSTATUS
NTAPI
RxgkPortDriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED;
    //__debugbreak();
    return 0;
}


NTSTATUS
NTAPI
RxgkPortDispatchCreateDevice(_In_    PDEVICE_OBJECT DeviceObject,
                             _Inout_ PIRP Irp)
{
    UNIMPLEMENTED;
    //__debugbreak();
    return 0;
}

/*
 * @ UNIMPLEMENTED
 */
NTSTATUS
NTAPI
RxgkPortDispatchPnp(_In_ PDEVICE_OBJECT DeviceObject,
                    _In_ PVOID Tag)
{
    UNIMPLEMENTED;
    //__debugbreak();
    return 0;
}

/*
 * @ UNIMPLEMENTED
 */
PSTR
NTAPI
RxgkPortDispatchPower(_In_ PDEVICE_OBJECT DeviceObject,
                     _In_ PSTR MutableMessage)
{
    UNIMPLEMENTED;
    //__debugbreak();
    return MutableMessage;
}

/*
 * @ UNIMPLEMENTED
 */
NTSTATUS
NTAPI
RxgkPortDispatchIoctl(_In_    PDEVICE_OBJECT DeviceObject,
                      _Inout_ IRP *Irp)
{
    UNIMPLEMENTED;
    //__debugbreak();
    return 0;
}

/*
 * @ UNIMPLEMENTED
 */
NTSTATUS
NTAPI
RxgkPortDispatchInternalIoctl(_In_ PDEVICE_OBJECT DeviceObject,
                             _Inout_ IRP *Irp)
{
    UNIMPLEMENTED;
    //__debugbreak();
    return 0;
}

/*
 * @ UNIMPLEMENTED
 */
NTSTATUS
NTAPI
RxgkPortDispatchSystemControl(_In_ PDEVICE_OBJECT DeviceObject,
                              _In_ PVOID Tag)
{
    UNIMPLEMENTED;
    //__debugbreak();
    return 0;
}

/*
 * @ UNIMPLEMENTED
 */
NTSTATUS
NTAPI
RxgkPortDispatchCloseDevice(_In_ PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    //__debugbreak();
    return 0;
}

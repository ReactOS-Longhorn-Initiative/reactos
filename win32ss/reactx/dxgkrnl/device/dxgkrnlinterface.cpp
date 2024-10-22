#include <rxgkrnl.h>

#include <debug.h>

extern PRXGK_PRIVATE_EXTENSION RxgkDriverExtension;
DXGKRNL_INTERFACE DxgkrnlInterface;

CODE_SEG("PAGE")
NTSTATUS
RxgkpQueryInterface(
    _In_ PRXGK_PRIVATE_EXTENSION RxgkpExtension,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Size)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;

    PAGED_CODE();

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       RxgkpExtension->MiniportPdo,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.InterfaceType = Guid;
    Stack->Parameters.QueryInterface.Version = 1;
    Stack->Parameters.QueryInterface.Size = Size;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)Interface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(RxgkpExtension->MiniportPdo, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    return Status;
}


/*
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dispmprt/nc-dispmprt-dxgkcb_eval_acpi_method
 * @ UNIMPLEMENTED
 */
NTSTATUS
APIENTRY
RxgkCbEvalAcpiMethod(_In_ HANDLE DeviceHandle,
                    _In_ ULONG DeviceUid,
                    _In_reads_bytes_(AcpiInputSize) PACPI_EVAL_INPUT_BUFFER_COMPLEX AcpiInputBuffer,
                    _In_range_(>=, sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX)) ULONG AcpiInputSize,
                    _Out_writes_bytes_(AcpiOutputSize) PACPI_EVAL_OUTPUT_BUFFER AcpiOutputBuffer,
                    _In_range_(>=, sizeof(ACPI_EVAL_OUTPUT_BUFFER)) ULONG AcpiOutputSize)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
NTAPI
DxgkrnlSetupResourceList(_Inout_ PCM_RESOURCE_LIST* ResourceList)
{
    NTSTATUS Status;
    PCI_SLOT_NUMBER PciSlotNumber;
    PCI_COMMON_CONFIG Config;
    ULONG ReturnedLength;



    PciSlotNumber.u.AsULONG = RxgkDriverExtension->SystemIoSlotNumber;


    if (RxgkDriverExtension->MiniportPdo != NULL)
    {
        PciSlotNumber.u.AsULONG = RxgkDriverExtension->SystemIoSlotNumber;

        ReturnedLength = HalGetBusData(PCIConfiguration,
                                       RxgkDriverExtension->SystemIoBusNumber,
                                       PciSlotNumber.u.AsULONG,
                                       &Config,
                                       sizeof(Config));

        if (ReturnedLength != sizeof(Config))
        {
            __debugbreak();
            return STATUS_NO_MEMORY;
        }
    }
    else
    {
        __debugbreak();
    }

    Status = HalAssignSlotResources(&RxgkDriverExtension->RegistryPath,
                                            NULL,
                                            RxgkDriverExtension->MiniportDriverObject,
                                            RxgkDriverExtension->MiniportDriverObject->DeviceObject,
                                            RxgkDriverExtension->AdapterInterfaceType,
                                            RxgkDriverExtension->SystemIoBusNumber,
                                            PciSlotNumber.u.AsULONG,
                                            ResourceList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HalAssignSlotResources failed with status %x.\n",Status);
        __debugbreak();
        return Status;
    }
    /* ******************************************************************/
    DPRINT1("ResourceList loc %X\n", ResourceList);
    return 0;
}


/**
 * @brief Fills out the DXGK_DEVICE_INFO parameter allocated
 *  by a miniport driver
 *
 * @param DeviceHandle HANDLE Obtained via the DXGKRNL_INTERFACE passed to miniport
 *
 * @param DeviceInfo Strucutre that includes many use bits of information for miniports, Including the IO Ranges
 *
 * @return NTSTATUS
 */
NTSTATUS
APIENTRY
RxgkCbGetDeviceInformation(_In_ HANDLE DeviceHandle,
                           _Out_ PDXGK_DEVICE_INFO DeviceInfo)
{
    PHYSICAL_ADDRESS PhyNull, HighestPhysicalAddress;
    NTSTATUS Status;

    PCM_RESOURCE_LIST TranslatedResourceList;
    PhyNull.QuadPart = NULL;
    HighestPhysicalAddress.QuadPart = 0x40000000;


    Status = DxgkrnlSetupResourceList(&TranslatedResourceList);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("DxgkCbGetDeviceInformation: Failed with status: %X\n", Status);
    }
    DPRINT1("DxgkCbGetDeviceInformation: Called\n");
    DeviceInfo->TranslatedResourceList = TranslatedResourceList;
    DeviceInfo->MiniportDeviceContext = RxgkDriverExtension->MiniportFdo;
    DeviceInfo->PhysicalDeviceObject = RxgkDriverExtension->MiniportPdo;
    DeviceInfo->DockingState = DockStateUnsupported;
    DeviceInfo->SystemMemorySize.QuadPart = 0x30000000;
    DeviceInfo->AgpApertureBase = PhyNull;
    DeviceInfo->AgpApertureSize = 0;
    DeviceInfo->DeviceRegistryPath = RxgkDriverExtension->RegistryPath;
    DeviceInfo->HighestPhysicalAddress = HighestPhysicalAddress;
    DeviceInfo->MiniportDeviceContext = RxgkDriverExtension->MiniportContext;
    return STATUS_SUCCESS;
}

/*
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dispmprt/nc-dispmprt-dxgkcb_indicate_child_status
 * @ UNIMPLEMENTED
 */
NTSTATUS
APIENTRY
RxgkCbIndicateChildStatus(_In_ HANDLE DeviceHandle,
                               _In_ PDXGK_CHILD_STATUS ChildStatus)
{
    //TODO: Implement meh
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS NTAPI
MapPhysicalMemory(
   IN HANDLE Process,
   IN PHYSICAL_ADDRESS PhysicalAddress,
   IN ULONG SizeInBytes,
   IN ULONG Protect,
   IN OUT PVOID *VirtualAddress  OPTIONAL)
{
   OBJECT_ATTRIBUTES ObjAttribs;
   UNICODE_STRING UnicodeString;
   HANDLE hMemObj;
   NTSTATUS Status;
   SIZE_T Size;

   /* Initialize object attribs */
   RtlInitUnicodeString(&UnicodeString, L"\\Device\\PhysicalMemory");
   InitializeObjectAttributes(&ObjAttribs,
                              &UnicodeString,
                              OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                              NULL, NULL);

   /* Open physical memory section */
   Status = ZwOpenSection(&hMemObj, SECTION_ALL_ACCESS, &ObjAttribs);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("ZwOpenSection() failed! (0x%x)\n", Status);
      return Status;
   }

   /* Map view of section */
   Size = SizeInBytes;
   Status = ZwMapViewOfSection(hMemObj,
                               Process,
                               VirtualAddress,
                               0,
                               Size,
                               (PLARGE_INTEGER)(&PhysicalAddress),
                               &Size,
                               ViewUnmap,
                               0,
                               Protect);
   ZwClose(hMemObj);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("ZwMapViewOfSection() failed! (0x%x)\n", Status);
   }

   return Status;
}


NTSTATUS
APIENTRY
RxgkCbMapMemory(_In_ HANDLE DeviceHandle,
                _In_ PHYSICAL_ADDRESS TranslatedAddress,
                _In_ ULONG Length,
                _In_ BOOLEAN InIoSpace,
                _In_ BOOLEAN MapToUserMode,
                _In_ MEMORY_CACHING_TYPE CacheType,
                _Outptr_ PVOID *VirtualAddress)
{
   NTSTATUS Status;
       ULONG AddressSpace = InIoSpace;
    PHYSICAL_ADDRESS CompleteAddress;
    if (HalTranslateBusAddress(
          RxgkDriverExtension->AdapterInterfaceType,
          RxgkDriverExtension->SystemIoBusNumber,
          TranslatedAddress,
          &AddressSpace,
          &CompleteAddress) == FALSE)
   {
        __debugbreak();

      return NULL;
   }

    DPRINT1("DxgkCbMapMemory Entry\n");
    if (InIoSpace == TRUE)
    {
        DPRINT1("Mapping InIoSpace\n");
        *VirtualAddress = (PVOID)CompleteAddress.LowPart;
    }
    else if(MapToUserMode)
    {

                    /* Map to userspace */
                Status = MapPhysicalMemory((HANDLE)0xFFFFFFFFFFFFFFFF,
                               CompleteAddress,
                               Length,
                               PAGE_READWRITE/* | PAGE_WRITECOMBINE*/,
                               VirtualAddress);

        if (!NT_SUCCESS(Status))
         {
            DPRINT1("DxgkCbMapMemory: MapPhysicalMemory() failed! (0x%x)\n", Status);
            *VirtualAddress =  NULL;
            return Status;
         }

    }
    else
    {
        *VirtualAddress = MmMapIoSpace(CompleteAddress, Length, CacheType);
    }
    

    if (*VirtualAddress == NULL)
    {
        DPRINT1("VirtualAddress is still NULL - reverting to fallback\n");
        //* final fallback
          *VirtualAddress = (PVOID)CompleteAddress.LowPart;
        return STATUS_SUCCESS;
    }
    DPRINT1("DxgkCbMapMemory Exit\n");
    return STATUS_SUCCESS;
}


NTSTATUS
APIENTRY
RxgkCbQueryServices(_In_ HANDLE DeviceHandle,
                    _In_ DXGK_SERVICES ServicesType,
                    _Inout_ PINTERFACE Interface)
{

    switch(ServicesType)
    {
        case DxgkServicesAgp:
            DPRINT1("DxgkCbQuerySercices: requested DxgkServicesAgp services.\n");
            break;
        case DxgkServicesDebugReport:
            DPRINT1("DxgkCbQuerySercices: requested DxgkServicesDebugReport services.\n");
            break;
        case DxgkServicesTimedOperation:
            DPRINT1("DxgkCbQuerySercices: requested DxgkServicesTimedOperation services.\n");
            break;
        case DxgkServicesSPB:
            DPRINT1("DxgkCbQuerySercices: requested DxgkServicesSPB services.\n");
            break;
        case DxgkServicesBDD:
            DPRINT1("DxgkCbQuerySercices: requested DxgkServicesBDD services.\n");
            break;
        case DxgkServicesFirmwareTable:
            DPRINT1("DxgkCbQuerySercices: requested DxgkServicesFirmwareTable services.\n");
            break;
        case DxgkServicesIDD:
            DPRINT1("DxgkCbQuerySercices: requested DxgkServicesIDD services.\n");
            break;
    }
    //TODO: Implement meh
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkCbIsDevicePresent(_In_ HANDLE DeviceHandle,
                      _In_ PPCI_DEVICE_PRESENCE_PARAMETERS DevicePresenceParameters,
                      _Out_ PBOOLEAN DevicePresent)
{
    //TODO: Implement meh
    UNIMPLEMENTED;
    //__debugbreak();
    return STATUS_UNSUCCESSFUL;
}

VOID*
APIENTRY
CALLBACK
RxgkCbGetHandleData(_In_ const PDXGKARGCB_GETHANDLEDATA GetHandleData)
{
    UNIMPLEMENTED;
    return NULL;
}

NTSTATUS
APIENTRY
RxgkCbUnmapMemory(_In_ HANDLE DeviceHandle,
                       _In_ PVOID VirtualAddress)
{
    //TODO: Implement meh
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

D3DKMT_HANDLE
APIENTRY
CALLBACK
RxgkCbGetHandleParent(_In_ D3DKMT_HANDLE hAllocation)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

D3DKMT_HANDLE
APIENTRY
CALLBACK
RxgkCbEnumHandleChildren(_In_ const PDXGKARGCB_ENUMHANDLECHILDREN EnumHandleChildren)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
CALLBACK
RxgkCbQueryMonitorInterface(_In_ const HANDLE                          hAdapter,
                            _In_ const DXGK_MONITOR_INTERFACE_VERSION  MonitorInterfaceVersion,
                            _Outptr_ const PDXGK_MONITOR_INTERFACE*    ppMonitorInterface)
{
    //TODO: Implement meh
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
CALLBACK
RxgkCbGetCaptureAddress(_Inout_ DXGKARGCB_GETCAPTUREADDRESS* GetCaptureAddress)
{
    //TODO: Implement meh
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

VOID
APIENTRY
RxgkCbLogEtwEvent(_In_ CONST LPCGUID EventGuid,
                       _In_ UCHAR Type,
                       _In_ USHORT EventBufferSize,
                       _In_reads_bytes_(EventBufferSize) PVOID EventBuffer)
{
   //TODO: Implement meh
    UNIMPLEMENTED;
}

NTSTATUS
APIENTRY
NTAPI
RxgkCbExcluseAdapterAccess(_In_ HANDLE DeviceHandle,
                           _In_ ULONG Attributes,
                           _In_ DXGKDDI_PROTECTED_CALLBACK rotectedCallback,
                           _In_ PVOID ProtectedCallbackContext)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
APIENTRY
CALLBACK
RxgkCbQueryVidPnInterface(_In_ const D3DKMDT_HVIDPN                   hVidPn,
                           _In_ const DXGK_VIDPN_INTERFACE_VERSION    VidPnInterfaceVersion,
                           _Outptr_ const PDXGK_VIDPN_INTERFACE       ppVidPnInterface)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
APIENTRY
RxgkCbQueueDpc(_In_ HANDLE DeviceHandle)
{
    UNIMPLEMENTED;
    __debugbreak();
    return FALSE;
}

NTSTATUS
APIENTRY
RxgkCbSynchronizeExecution(_In_ HANDLE DeviceHandle,
                           _In_ PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
                           _In_ PVOID Context,
                           _In_ ULONG MessageNumber,
                           _Out_ PBOOLEAN ReturnValue)
{
{
    DPRINT1("RxgkCbSynchronizeExecution: ENtry\n");
//    *ReturnValue = KeSynchronizeExecution(RxgkDriverExtension->InterruptObject, SynchronizeRoutine, Context);
   *ReturnValue = 0;
    return STATUS_SUCCESS;
}

}


BOOLEAN NTAPI
RxgkpInterruptRoutine(
   IN struct _KINTERRUPT *Interrupt,
   IN PVOID ServiceContext)
{
    UNIMPLEMENTED;
    __debugbreak();
    return FALSE;
}

VOID
APIENTRY
CALLBACK
RxgkCbNotifyDpc(_In_ const HANDLE hAdapter)
{
    DPRINT("WARNING!!! Scheduler is UNIMPLEMENTED: Event DxgkCbNotifyDpc detected\n");
}

VOID
APIENTRY
CALLBACK
RxgkCbNotifyInterrupt(_In_ const HANDLE hAdapter,
                      _In_ const PDXGKARGCB_NOTIFY_INTERRUPT_DATA NotifyInterruptData)
{
    switch (NotifyInterruptData->InterruptType)
    {
        case DXGK_INTERRUPT_DMA_COMPLETED:
            DPRINT("WARNING!!! Scheduler is UNIMPLEMENTED: Event DXGK_INTERRUPT_DMA_COMPLETED detected\n");
            break;
        case DXGK_INTERRUPT_DMA_PREEMPTED:
            DPRINT("WARNING!!! Scheduler is UNIMPLEMENTED: Event DXGK_INTERRUPT_DMA_PREEMPTED detected\n");
            break;
        case DXGK_INTERRUPT_CRTC_VSYNC:
            DPRINT("WARNING!!! Scheduler is UNIMPLEMENTED: Event DXGK_INTERRUPT_CRTC_VSYNC detected\n");
            break;
        case DXGK_INTERRUPT_DMA_FAULTED:
            DPRINT("WARNING!!! Scheduler is UNIMPLEMENTED: Event DXGK_INTERRUPT_DMA_FAULTED detected\n");
            break;
    }
}


NTSTATUS
APIENTRY
RxgkCbReadDeviceSpace(_In_ HANDLE DeviceHandle,
                      _In_ ULONG DataType,
                      _Out_writes_bytes_to_(Length, *BytesRead) PVOID Buffer,
                      _In_ ULONG Offset,
                      _In_ ULONG Length,
                      _Out_ PULONG BytesRead)
{
    switch (DataType)
    {
        case DXGK_WHICHSPACE_BRIDGE:
            DPRINT1("DxgkCbReadDeviceSpace: DXGK_WHICHSPACE_BRIDGE\n");
            UNIMPLEMENTED;
            break;
        case DXGK_WHICHSPACE_CONFIG:
            DPRINT1("DxgkCbReadDeviceSpace: DXGK_WHICHSPACE_CONFIG\n");
            *BytesRead = (RxgkDriverExtension->BusInterface.GetBusData)(RxgkDriverExtension->BusInterface.Context,
                                                                   PCI_WHICHSPACE_CONFIG,
                                                                   Buffer,
                                                                   Offset,
                                                                   Length);
             DPRINT1("DxgkCbReadDeviceSpace: DXGK_WHICHSPACE_CONFIG - SUCCESS\n");
            break;
        case DXGK_WHICHSPACE_MCH:
            DPRINT1("DxgkCbReadDeviceSpace: DXGK_WHICHSPACE_MCH");
            UNIMPLEMENTED;
            break;
        case DXGK_WHICHSPACE_ROM:
            DPRINT1("DxgkCbReadDeviceSpace: DXGK_WHICHSPACE_ROM");
            UNIMPLEMENTED;
            break;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
APIENTRY
RxgkCbWriteDeviceSpace(_In_ HANDLE DeviceHandle,
                       _In_ ULONG DataType,
                       _In_reads_bytes_(Length) PVOID Buffer,
                       _In_ ULONG Offset,
                       _In_ ULONG Length,
                       _Out_ _Out_range_(<=, Length) PULONG BytesWritten)
{
    switch (DataType)
    {
        case DXGK_WHICHSPACE_BRIDGE:
            DPRINT1("DxgkCbWriteDeviceSpace: DXGK_WHICHSPACE_BRIDGE\n");
            UNIMPLEMENTED;
            break;
        case DXGK_WHICHSPACE_CONFIG:
            DPRINT1("DxgkCbWriteDeviceSpace: DXGK_WHICHSPACE_CONFIG");
            UNIMPLEMENTED;
            break;
        case DXGK_WHICHSPACE_MCH:
            DPRINT1("DxgkCbWriteDeviceSpace: DXGK_WHICHSPACE_MCH");
            UNIMPLEMENTED;
            break;
        case DXGK_WHICHSPACE_ROM:
            DPRINT1("DxgkCbWriteDeviceSpace: DXGK_WHICHSPACE_ROM");
            UNIMPLEMENTED;
            break;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
RxgkpSetupDxgkrnl(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    DXGKRNL_INTERFACE DxgkrnlInterfaceLoc = {0};
    DxgkrnlInterface.Size = sizeof(DXGKRNL_INTERFACE);
    DxgkrnlInterface.Version = DXGKDDI_INTERFACE_VERSION_VISTA;
    DxgkrnlInterface.DeviceHandle = (HANDLE)DriverObject;
    DxgkrnlInterface.DxgkCbEvalAcpiMethod = RxgkCbEvalAcpiMethod;
    DxgkrnlInterface.DxgkCbGetDeviceInformation = RxgkCbGetDeviceInformation;
    DxgkrnlInterface.DxgkCbIndicateChildStatus = RxgkCbIndicateChildStatus;
    DxgkrnlInterface.DxgkCbMapMemory = RxgkCbMapMemory;
    DxgkrnlInterface.DxgkCbQueueDpc = RxgkCbQueueDpc;
    DxgkrnlInterface.DxgkCbQueryServices = RxgkCbQueryServices;
    DxgkrnlInterface.DxgkCbReadDeviceSpace = RxgkCbReadDeviceSpace;
    DxgkrnlInterface.DxgkCbSynchronizeExecution = RxgkCbSynchronizeExecution;
    DxgkrnlInterface.DxgkCbUnmapMemory = RxgkCbUnmapMemory;
    DxgkrnlInterface.DxgkCbWriteDeviceSpace = RxgkCbWriteDeviceSpace;
    DxgkrnlInterface.DxgkCbIsDevicePresent = RxgkCbIsDevicePresent;
    DxgkrnlInterface.DxgkCbGetHandleData = RxgkCbGetHandleData;
    DxgkrnlInterface.DxgkCbGetHandleParent = RxgkCbGetHandleParent;
    DxgkrnlInterface.DxgkCbEnumHandleChildren = RxgkCbEnumHandleChildren;
    DxgkrnlInterface.DxgkCbNotifyInterrupt = RxgkCbNotifyInterrupt;
    DxgkrnlInterface.DxgkCbNotifyDpc = RxgkCbNotifyDpc;
    DxgkrnlInterface.DxgkCbQueryVidPnInterface = RxgkCbQueryVidPnInterface;
    DxgkrnlInterface.DxgkCbQueryMonitorInterface = RxgkCbQueryMonitorInterface;
    DxgkrnlInterface.DxgkCbGetCaptureAddress = RxgkCbGetCaptureAddress;
    DxgkrnlInterface.DxgkCbLogEtwEvent = RxgkCbLogEtwEvent;
    DxgkrnlInterface.DxgkCbExcludeAdapterAccess = RxgkCbExcluseAdapterAccess;
    DPRINT1("Targetting version: %X\n", DxgkrnlInterface.Version);
    return STATUS_SUCCESS;
}


/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpirp.c
 * PURPOSE:         Code for IRP_MJ_PNP requests
 * PROGRAMMERS:     
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* DATA **********************************************************************/

typedef struct _LOCK_DEVICES_FOR_REMOVE
{
    PDEVICE_OBJECT RemoveDeviceObject;
    PDEVICE_OBJECT FileSystemDeviceObject;
} LOCK_DEVICES_FOR_REMOVE, *PLOCK_DEVICES_FOR_REMOVE;

typedef struct _QUERY_REMOVE_DEVICE_CONTEXT
{
    PDRIVER_OBJECT DriverObject;
    KEVENT Event;
    NTSTATUS CompletionStatus;
} QUERY_REMOVE_DEVICE_CONTEXT, *PQUERY_REMOVE_DEVICE_CONTEXT;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
IopSynchronousCall(IN PDEVICE_OBJECT DeviceObject,
                   IN PIO_STACK_LOCATION IoStackLocation,
                   OUT PVOID* Information)
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    IO_STATUS_BLOCK IoStatusBlock;
    KEVENT Event;
    NTSTATUS Status;
    PDEVICE_OBJECT TopDeviceObject;
    PAGED_CODE();

    /* Call the top of the device stack */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);
    DPRINT("IopSynchronousCall: [%X] Device %p, [%X] TopDevice %p\n", DeviceObject->Characteristics, DeviceObject, TopDeviceObject->Characteristics, TopDeviceObject);

    Irp = IoAllocateIrp(TopDeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        /* Remove the reference */
        DPRINT1("IopSynchronousCall: return STATUS_INSUFFICIENT_RESOURCES\n");
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize to failure */
    Irp->IoStatus.Status = IoStatusBlock.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = IoStatusBlock.Information = 0;

    if (IoStackLocation->MinorFunction == IRP_MN_FILTER_RESOURCE_REQUIREMENTS)
    {
        /* Copy the resource requirements list into the IOSB */
        Irp->IoStatus.Information =
        IoStatusBlock.Information = (ULONG_PTR)IoStackLocation->Parameters.FilterResourceRequirements.IoResourceRequirementList;
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp->UserIosb = &IoStatusBlock;
    Irp->UserEvent = &Event;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IoQueueThreadIrp(Irp);

    /* Copy-in the stack */
    IrpStack = IoGetNextIrpStackLocation(Irp);
    *IrpStack = *IoStackLocation;

    //DPRINT("IopSynchronousCall: [%X] IoCallDriver(%X %X)\n", TopDeviceObject->Characteristics, TopDeviceObject, Irp);
    Status = IoCallDriver(TopDeviceObject, Irp);
    //DPRINT("IopSynchronousCall: [%X] Status %X\n", TopDeviceObject->Characteristics, Status);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
        DPRINT("IopSynchronousCall: Status %X\n", Status);
    }

    ASSERT(Status != STATUS_PENDING);

    ObDereferenceObject(TopDeviceObject);

    if (Information)
    {
        *Information = (PVOID)IoStatusBlock.Information;
        DPRINT("SyncCall: IoStatusBlock.Information %X\n", IoStatusBlock.Information);
    }

    DPRINT("IopSynchronousCall: return Status %X\n", Status);
    return Status;
}

NTSTATUS
NTAPI
PiAsynchronousCall(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIO_STACK_LOCATION InIoStack,
    _In_ PIO_COMPLETION_ROUTINE CompletionRoutine,
    _In_ PQUERY_REMOVE_DEVICE_CONTEXT QueryContext)
{
    PDEVICE_OBJECT TopDeviceObject;
    PIO_STACK_LOCATION IoStack;
    PIRP Irp;
    NTSTATUS Status;

    PAGED_CODE();

    /* Get the top of the device stack */
    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    /* Allocate an IRP */
    Irp = IoAllocateIrp(TopDeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        /* Remove the reference */
        DPRINT1("PiAsynchronousCall: return STATUS_INSUFFICIENT_RESOURCES\n");
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize to failure */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    /* Set them up */
    Irp->UserIosb = NULL;
    Irp->UserEvent = NULL;

    /* Queue the IRP */
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->RequestorMode = KernelMode;

    IoStack = Irp->Tail.Overlay.CurrentStackLocation;
    RtlCopyMemory(&IoStack[-1], InIoStack, sizeof(IO_STACK_LOCATION));

    IoSetCompletionRoutine(Irp,
                           CompletionRoutine,
                           QueryContext,
                           TRUE,
                           TRUE,
                           TRUE);

    /* Call the driver */
    Status = IoCallDriver(TopDeviceObject, Irp);

    /* Remove the reference */
    ObDereferenceObject(TopDeviceObject);

    DPRINT("PiAsynchronousCall: return Status - %X\n", Status);
    return Status;
}

NTSTATUS
NTAPI
IopQueryDeviceRelations(
    _In_ DEVICE_RELATION_TYPE RelationsType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PDEVICE_RELATIONS * OutPendingDeviceRelations)
{
    PDEVICE_NODE DeviceNode;
    IO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    RtlZeroMemory(&IoStack, sizeof(IO_STACK_LOCATION));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;

    IoStack.Parameters.QueryDeviceRelations.Type = RelationsType;

    Status = IopSynchronousCall(DeviceObject, &IoStack, (PVOID *)OutPendingDeviceRelations);

    if (RelationsType != BusRelations)
    {
        DPRINT("IopQueryDeviceRelations: (%X) Status %X\n", RelationsType, Status);
        return Status;
    }

    DeviceNode = IopGetDeviceNode(DeviceObject);
    DeviceNode->CompletionStatus = Status;

    PipSetDevNodeState(DeviceNode, DeviceNodeEnumerateCompletion, NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PpIrpQueryID(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BUS_QUERY_ID_TYPE IdType,
    _Out_ PWCHAR * OutID)
{
    IO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpIrpQueryID: Device %p, Type %X\n", DeviceObject, IdType);

    ASSERT(IdType == BusQueryDeviceID ||
           IdType == BusQueryInstanceID ||
           IdType == BusQueryHardwareIDs ||
           IdType == BusQueryCompatibleIDs ||
           IdType == BusQueryDeviceSerialNumber);
    
    *OutID = NULL;

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_ID;

    IoStack.Parameters.QueryId.IdType = IdType;

    Status = IopSynchronousCall(DeviceObject, &IoStack, (PVOID *)OutID);
    if (!NT_SUCCESS(Status))
    {
        if (*OutID != NULL)
        {
           #if DBG
             DPRINT1("PpIrpQueryID: Device %p, Type %X\n", DeviceObject, IdType);
             DPRINT1("Dumping Nodes:\n");
             PipDumpDeviceNodes(NULL, 1+2+4+8, 0);
             ASSERT(*OutID == NULL);
           #endif

            *OutID = NULL;
        }

        DPRINT("PpIrpQueryID: return Status %X\n", Status);
        return Status;
    }

    if (*OutID == NULL)
    {
        DPRINT("PpIrpQueryID: STATUS_NOT_SUPPORTED\n");
        Status = STATUS_NOT_SUPPORTED;
    }

    DPRINT("PpIrpQueryID: FIXME PiFailQueryID\n");
    //if ( PnPBootDriversInitialized && PiFailQueryID && RtlRandom((PULONG)&CurrentTime.LowPart) % 10 > 7 )

    return Status;
}

NTSTATUS
NTAPI
PpIrpQueryCapabilities(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PDEVICE_CAPABILITIES DeviceCapabilities)
{
    IO_STACK_LOCATION IoStack;

    PAGED_CODE();
    DPRINT("PpIrpQueryCapabilities: DeviceCapabilities %p\n", DeviceCapabilities);

    RtlZeroMemory(DeviceCapabilities, sizeof(DEVICE_CAPABILITIES));

    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;

    DeviceCapabilities->Address = -1;
    DeviceCapabilities->UINumber = -1;

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_CAPABILITIES;

    IoStack.Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    return IopSynchronousCall(DeviceObject, &IoStack, NULL);
}

NTSTATUS
NTAPI
IopQueryDeviceState(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PNP_DEVICE_STATE *OutState)
{
    PNP_DEVICE_STATE State;
    IO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopQueryDeviceState: DeviceObject - %p\n", DeviceObject);

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_PNP_DEVICE_STATE;

    Status = IopSynchronousCall(DeviceObject, &IoStack, (PVOID *)&State);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopQueryDeviceState: Status - %X\n", Status);
        return Status;
    }

    *OutState = State;

    return Status;
}

NTSTATUS
NTAPI
PpIrpQueryDeviceText(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_TEXT_TYPE DeviceTextType,
    _In_ LCID LocaleId,
    _Out_ PWCHAR * OutDeviceText)
{
    NTSTATUS Status;
    IO_STACK_LOCATION IoStack;

    PAGED_CODE();
    DPRINT("PpIrpQueryDeviceText: DeviceObject - %p\n", DeviceObject);

    ASSERT(DeviceTextType == DeviceTextDescription ||
           DeviceTextType == DeviceTextLocationInformation);

    *OutDeviceText = NULL;

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_DEVICE_TEXT;

    IoStack.Parameters.QueryDeviceText.DeviceTextType = DeviceTextType;
    IoStack.Parameters.QueryDeviceText.LocaleId = LocaleId;

    Status = IopSynchronousCall(DeviceObject, &IoStack, (PVOID *)OutDeviceText);

    ASSERT(NT_SUCCESS(Status) || (*OutDeviceText == NULL));

    if (!NT_SUCCESS(Status))
    {
        *OutDeviceText = NULL;
        return Status;
    }

    if (*OutDeviceText == NULL)
    {
        Status = STATUS_NOT_SUPPORTED;
    }

    return Status;
}

NTSTATUS
NTAPI
PpIrpQueryResourceRequirements(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST * IoResource)
{
    NTSTATUS Status;
    IO_STACK_LOCATION IoStack;

    PAGED_CODE();
    DPRINT("PpIrpQueryResourceRequirements: DeviceObject - %p\n", DeviceObject);

    *IoResource = NULL;

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_RESOURCE_REQUIREMENTS;

    Status = IopSynchronousCall(DeviceObject, &IoStack, (PVOID *)IoResource);

    ASSERT(NT_SUCCESS(Status) || (*IoResource == NULL));

    if (!NT_SUCCESS(Status))
    {
        *IoResource = NULL;
        return Status;
    }

    if (*IoResource == NULL)
    {
        Status = STATUS_NOT_SUPPORTED;
    }

    return Status;
}

NTSTATUS
NTAPI
PpIrpQueryBusInformation(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PPNP_BUS_INFORMATION * OutInformation)
{
    IO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpIrpQueryBusInformation: DeviceObject %p\n", DeviceObject);

    *OutInformation = NULL;
    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_BUS_INFORMATION;

    Status = IopSynchronousCall(DeviceObject, &IoStack, (PVOID *)OutInformation);

    if (!NT_SUCCESS(Status))
    {
        *OutInformation = NULL;
    }

    return Status;
}

NTSTATUS
NTAPI
PpIrpQueryResources(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PCM_RESOURCE_LIST * OutResourceList,
    _Out_ PULONG OutSize)
{
    IO_STACK_LOCATION IoStack;
    PDEVICE_NODE DeviceNode;
    ULONG ConfigTypes;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpIrpQueryResources: DeviceObject %p\n", DeviceObject);

    *OutResourceList = NULL;
    *OutSize = 0;

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    DeviceNode = IopGetDeviceNode(DeviceObject);

    if (DeviceNode->Flags & DNF_MADEUP)
    {
        DPRINT("PpIrpQueryResources: DeviceNode->Flags & DNF_MADEUP\n");

        ConfigTypes = PIP_CONFIG_TYPE_BOOT +
                      PIP_CONFIG_TYPE_FORCED +
                      PIP_CONFIG_TYPE_ALLOC;

        Status = IopGetDeviceResourcesFromRegistry(DeviceObject,
                                                   FALSE, // PCM_RESOURCE_LIST
                                                   ConfigTypes,
                                                   (PVOID *)OutResourceList,
                                                   OutSize);

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            return Status;
        }

        return STATUS_SUCCESS;
    }

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_RESOURCES;

    Status = IopSynchronousCall(DeviceObject, &IoStack, (PVOID*)OutResourceList);

    if (Status == STATUS_NOT_SUPPORTED)
    {
        DPRINT("PpIrpQueryResources: Status == STATUS_NOT_SUPPORTED\n");
        *OutResourceList = NULL;
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status))
    {
        *OutSize = PnpDetermineResourceListSize(*OutResourceList);
         DPRINT("PpIrpQueryResources: OutSize %X\n", *OutSize);
    }

    return Status;
}

NTSTATUS
NTAPI
IopQueryLegacyBusInformation(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ GUID * OutBusTypeGuid,
    _Out_ INTERFACE_TYPE * OutInterfaceType,
    _Out_ PULONG OutBusNumber)
{
    PLEGACY_BUS_INFORMATION BusInfo;
    PDEVICE_NODE ParentDeviceNode;
    PDEVICE_NODE DeviceNode;
    IO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopQueryLegacyBusInformation: DeviceObject %p\n", DeviceObject);

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_LEGACY_BUS_INFORMATION;

    Status = IopSynchronousCall(DeviceObject, &IoStack, (PVOID *)&BusInfo);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopQueryLegacyBusInformation: Status %X\n", Status);
        return Status;
    }

    if (BusInfo)
    {
        if (OutBusTypeGuid)
            RtlCopyMemory(OutBusTypeGuid, &BusInfo->BusTypeGuid, sizeof(GUID));

        if (OutInterfaceType)
            *OutInterfaceType = BusInfo->LegacyBusType;

        if (OutBusNumber)
            *OutBusNumber = BusInfo->BusNumber;

        ExFreePool(BusInfo);

        return Status;
    }

    /* Error */

    DeviceNode = IopGetDeviceNode(DeviceObject);
    if (!DeviceNode)
        goto Exit;

    if (DeviceNode->ServiceName.Buffer)
    {
        DPRINT1("IopQueryLegacyBusInformation: Driver '%wZ'\n", &DeviceNode->ServiceName);
    }

    ParentDeviceNode = DeviceNode->Parent;
    if (!ParentDeviceNode)
        goto Exit;

    if (ParentDeviceNode->ServiceName.Buffer)
    {
        DPRINT1("IopQueryLegacyBusInformation: Parent driver '%wZ'\n", &ParentDeviceNode->ServiceName);
    }

Exit:

    DPRINT1("IopQueryLegacyBusInformation: return STATUS_SUCCESS and BusInfo == NULL!\n");

    ASSERT(BusInfo);
    return Status;
}

NTSTATUS
NTAPI
IopQueryResourceHandlerInterface(
    _In_ ULONG InterfaceType,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR InterfaceSpecificData,
    _Out_ PVOID * OutInterface)
{
    PLEGACY_DEVICE_DETECTION_INTERFACE LegacyInterface;
    PTRANSLATOR_INTERFACE TranslatorInterface;
    PARBITER_INTERFACE ArbiterInterface;
    PDEVICE_NODE DeviceNode;
    IO_STACK_LOCATION IoStack;
    PINTERFACE Interface;
    GUID GuidInterfaceType;
    NTSTATUS Status;
    USHORT InterfaceSize;

    PAGED_CODE();
    DeviceNode = IopGetDeviceNode(DeviceObject);
    DPRINT("IopQueryResourceHandlerInterface: InterfaceType - %X, DeviceNode - %p, InterfaceSpecificData - %X\n",
           InterfaceType, DeviceNode, InterfaceSpecificData);

    if (DeviceNode->DuplicatePDO == (PDEVICE_OBJECT)DeviceObject->DriverObject) // Yes, see IopFindLegacyDeviceNode()
    {
        ASSERT(FALSE);
        return STATUS_NOT_SUPPORTED;
    }

    if (!(DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE))
    {
        ASSERT(FALSE);
        return STATUS_NOT_SUPPORTED;
    }

    switch (InterfaceType)
    {
        case IOP_RES_HANDLER_TYPE_TRANSLATOR:
            InterfaceSize = sizeof(TRANSLATOR_INTERFACE);
            break;

        case IOP_RES_HANDLER_TYPE_ARBITER:
            InterfaceSize = sizeof(ARBITER_INTERFACE);
            break;

        case IOP_RES_HANDLER_TYPE_LEGACY:
            InterfaceSize = sizeof(LEGACY_DEVICE_DETECTION_INTERFACE);
            break;

        default:
            ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
    }

    Interface = ExAllocatePoolWithTag(PagedPool, InterfaceSize, '  pP');

    if (!Interface)
    {
        ASSERT(FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Interface, InterfaceSize);
    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_INTERFACE;

    Interface->Version = 0;
    Interface->Size = InterfaceSize;

    switch (InterfaceType)
    {
        case IOP_RES_HANDLER_TYPE_TRANSLATOR:
            RtlCopyMemory(&GuidInterfaceType, &GUID_TRANSLATOR_INTERFACE_STANDARD, sizeof(GUID));
            DPRINT("IopQueryResourceHandlerInterface: GUID_TRANSLATOR_INTERFACE_STANDARD\n");
            break;

        case IOP_RES_HANDLER_TYPE_ARBITER:
            RtlCopyMemory(&GuidInterfaceType, &GUID_ARBITER_INTERFACE_STANDARD, sizeof(GUID));
            DPRINT("IopQueryResourceHandlerInterface: GUID_ARBITER_INTERFACE_STANDARD\n");
            break;

        case IOP_RES_HANDLER_TYPE_LEGACY:
            RtlCopyMemory(&GuidInterfaceType, &GUID_LEGACY_DEVICE_DETECTION_STANDARD, sizeof(GUID));
            DPRINT("IopQueryResourceHandlerInterface: GUID_LEGACY_DEVICE_DETECTION_STANDARD\n");
            break;

        default:
            ASSERT(FALSE);
            return STATUS_INVALID_PARAMETER;
    }

    IoStack.Parameters.QueryInterface.InterfaceType = &GuidInterfaceType;

    IoStack.Parameters.QueryInterface.Size = InterfaceSize;
    IoStack.Parameters.QueryInterface.Version = 0;
    IoStack.Parameters.QueryInterface.Interface = Interface;
    IoStack.Parameters.QueryInterface.InterfaceSpecificData =
                                      ULongToPtr((ULONG)InterfaceSpecificData);

    Status = IopSynchronousCall(DeviceObject, &IoStack, NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopQueryResourceHandlerInterface: Status - %X\n", Status);
        goto ErrorExit;
    }

    switch (InterfaceType)
    {
        case IOP_RES_HANDLER_TYPE_TRANSLATOR:
        {
            TranslatorInterface = (PTRANSLATOR_INTERFACE)Interface;

            if (!TranslatorInterface->TranslateResources ||
                !TranslatorInterface->TranslateResourceRequirements)
            {
                DPRINT("IopQueryResourceHandlerInterface: TranslateResources - %p, TranslateResourceRequirements - %p\n",
                       TranslatorInterface->TranslateResources,
                       TranslatorInterface->TranslateResourceRequirements);

                ASSERT(!NT_SUCCESS(Status));
                Status = STATUS_UNSUCCESSFUL;
            }

            break;
        }
        case IOP_RES_HANDLER_TYPE_ARBITER:
        {
            ArbiterInterface = (PARBITER_INTERFACE)Interface;

            if (!ArbiterInterface->ArbiterHandler)
            {
                DPRINT("IopQueryResourceHandlerInterface: ArbiterHandler == NULL\n");

                ASSERT(!NT_SUCCESS(Status));
                Status = STATUS_UNSUCCESSFUL;
            }

            break;
        }
        case IOP_RES_HANDLER_TYPE_LEGACY:
        {
            LegacyInterface = (PLEGACY_DEVICE_DETECTION_INTERFACE)Interface;

            if (!LegacyInterface->LegacyDeviceDetection)
            {
                DPRINT("IopQueryResourceHandlerInterface: LegacyDeviceDetection == NULL\n");
                ASSERT(!NT_SUCCESS(Status));
                Status = STATUS_UNSUCCESSFUL;
            }

            break;
        }
        default:
        {
            DPRINT("IopQueryResourceHandlerInterface: Unknown InterfaceType - %X\n", InterfaceType);
            ASSERT(FALSE);
            Status = STATUS_INVALID_PARAMETER;
            goto ErrorExit;
        }
    }

    if (NT_SUCCESS(Status))
    {
        *OutInterface = Interface;
        return Status;
    }

ErrorExit:

    ExFreePoolWithTag(Interface, '  pP');
    return Status;
}

NTSTATUS
NTAPI
IopFilterResourceRequirementsCall(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST IoResources,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST * OutRequirementsList)
{
    PDEVICE_OBJECT TopDeviceObject;
    PIO_STACK_LOCATION IoStack;
    IO_STATUS_BLOCK IoSb;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("IopFilterResourceRequirementsCall: DeviceObject - %p, IoResources - %p\n",
           DeviceObject, IoResources);

    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    Irp = IoAllocateIrp(TopDeviceObject->StackSize, FALSE);

    if (!Irp)
    {
        DPRINT1("IopFilterResourceRequirementsCall: STATUS_INSUFFICIENT_RESOURCES\n");
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (IoResources)
    {
        IoSb.Status = STATUS_SUCCESS;
        Irp->IoStatus.Status = STATUS_SUCCESS;

        IoSb.Information = (ULONG_PTR)IoResources;
        Irp->IoStatus.Information = (ULONG_PTR)IoResources;
    }
    else
    {
        IoSb.Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    }

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp->UserIosb = &IoSb;
    Irp->UserEvent = &Event;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    KeEnterCriticalRegion();
    IoQueueThreadIrp(Irp);
    KeLeaveCriticalRegion();

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->Parameters.FilterResourceRequirements.IoResourceRequirementList = IoResources;

    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_FILTER_RESOURCE_REQUIREMENTS;

    Status = IoCallDriver(TopDeviceObject, Irp);

    if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
    {
        DPRINT("IopFilterResourceRequirementsCall: IRP_MN_FILTER_RESOURCE_REQUIREMENTS. Driver %wZ return Status - %X\n",
               &TopDeviceObject->DriverObject->DriverName, Status);

        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoSb.Status;
        }
    }

    *OutRequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST)IoSb.Information;

    ObDereferenceObject(TopDeviceObject);

    return Status;
}

NTSTATUS
NTAPI
IopStartDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    IO_STACK_LOCATION IoStack;

    PAGED_CODE();

    DPRINT("IopStartDevice: DeviceNode - %p\n", DeviceNode);
    DPRINT("IopStartDevice: ResourceList - %p, ResourceListTranslated - %p\n", DeviceNode->ResourceList, DeviceNode->ResourceListTranslated);
    DPRINT("IopStartDevice: InstancePath - %wZ, ServiceName - %wZ\n", &DeviceNode->InstancePath, &DeviceNode->ServiceName);

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_START_DEVICE;

    IoStack.Parameters.StartDevice.AllocatedResources = DeviceNode->ResourceList;
    IoStack.Parameters.StartDevice.AllocatedResourcesTranslated = DeviceNode->ResourceListTranslated;

    return IopSynchronousCall(DeviceNode->PhysicalDeviceObject, &IoStack, NULL);
}

NTSTATUS
NTAPI
IopQueryReconfiguration(
    _In_ UCHAR MinorFunction,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_NODE DeviceNode;
    IO_STACK_LOCATION IoStack;

    PAGED_CODE();
    DPRINT("IopQueryReconfiguration: MinorFunction - %X\n", MinorFunction);

    DeviceNode = IopGetDeviceNode(DeviceObject);

    if (MinorFunction == IRP_MN_STOP_DEVICE)
    {
        if (DeviceNode->State != DeviceNodeQueryStopped)
        {
            DPRINT("IopQueryReconfiguration: send IRP_MN_STOP_DEVICE to an unqueried device %wZ!\n", &DeviceNode->InstancePath);
            ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }
    else if (MinorFunction == IRP_MN_QUERY_STOP_DEVICE)
    {
        if (DeviceNode->State != DeviceNodeStarted)
        {
            DPRINT("IopQueryReconfiguration:  send IRP_MN_QUERY_STOP_DEVICE to an unstarted device %wZ!\n", &DeviceNode->InstancePath);
            ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }
    else if (MinorFunction == IRP_MN_CANCEL_STOP_DEVICE)
    {
        if (DeviceNode->State != DeviceNodeQueryStopped && DeviceNode->State != DeviceNodeStarted)
        {
            DPRINT("IopQueryReconfiguration:  send IRP_MN_CANCEL_STOP_DEVICE to an unqueried\\unstarted device %wZ!\n", &DeviceNode->InstancePath);
            ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = MinorFunction;

    return IopSynchronousCall(DeviceObject, &IoStack, NULL);
}

PDMA_ADAPTER
NTAPI
IoGetDmaAdapter(
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _In_ PDEVICE_DESCRIPTION DeviceDescription,
    _Inout_ PULONG NumberOfMapRegisters)
{
    NTSTATUS Status;
    ULONG ResultLength;
    BUS_INTERFACE_STANDARD BusInterface;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION Stack;
    DEVICE_DESCRIPTION PrivateDeviceDescription;
    PDMA_ADAPTER DmaAdapter = NULL;
    PDEVICE_OBJECT TopDeviceObject;
    PDEVICE_NODE DeviceNode;
    KEVENT Event;
    PIRP Irp;

    PAGED_CODE();
    DPRINT("IoGetDmaAdapter: PhysicalDeviceObject - %p, DeviceDescription - %p\n",
           PhysicalDeviceObject, DeviceDescription);

    /* Try to create DMA adapter through bus driver */
    if (!PhysicalDeviceObject)
    {
        DPRINT1("IoGetDmaAdapter: Error. PhysicalDeviceObject == NULL\n");
        goto ExitError;
    }

    DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
    if (!DeviceNode || DeviceNode->Flags & DNF_HAS_PROBLEM)
    {
        DPRINT1("IoGetDmaAdapter: PNP_DETECTED_FATAL_ERROR. DeviceNode - %p\n", DeviceNode);
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 2, (ULONG_PTR)PhysicalDeviceObject, 0, 0);
    }

    if (DeviceDescription->InterfaceType == PNPBus ||//15
        DeviceDescription->InterfaceType == InterfaceTypeUndefined)
    {
        RtlCopyMemory(&PrivateDeviceDescription,
                      DeviceDescription,
                      sizeof(PrivateDeviceDescription));

        Status = IoGetDeviceProperty(PhysicalDeviceObject,
                                     DevicePropertyLegacyBusType,
                                     sizeof(INTERFACE_TYPE),
                                     &PrivateDeviceDescription.InterfaceType,
                                     &ResultLength);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoGetDmaAdapter: Error. Status - %X\n", Status);
            ASSERT(Status == STATUS_OBJECT_NAME_NOT_FOUND);
            PrivateDeviceDescription.InterfaceType = Internal;//PnpDefaultInterfaceType
        }

        DeviceDescription = &PrivateDeviceDescription;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    TopDeviceObject = IoGetAttachedDeviceReference(PhysicalDeviceObject);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       TopDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
    {
        DPRINT1("IoGetDmaAdapter: Error ... return NULL\n");
        ASSERT(FALSE);
        return NULL;
    }

    RtlZeroMemory(&BusInterface, sizeof(BusInterface));

    Stack = IoGetNextIrpStackLocation(Irp);

    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    Stack->Parameters.QueryInterface.Version = 1;
    Stack->Parameters.QueryInterface.Interface = (PINTERFACE)&BusInterface;
    Stack->Parameters.QueryInterface.InterfaceType = &GUID_BUS_INTERFACE_STANDARD;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver(TopDeviceObject, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    ObDereferenceObject(TopDeviceObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoGetDmaAdapter: Error. Status - %X\n", Status);
        goto ExitError;
    }

    if (BusInterface.GetDmaAdapter)
    {
        DmaAdapter = BusInterface.GetDmaAdapter(BusInterface.Context,
                                                DeviceDescription,
                                                NumberOfMapRegisters);
    }

    BusInterface.InterfaceDereference(BusInterface.Context);

    if (DmaAdapter)
    {
        DPRINT("IoGetDmaAdapter: DmaAdapter - %X\n", DmaAdapter);
        return DmaAdapter;
    }

ExitError:

    /* Fall back to HAL */
    return HalGetDmaAdapter(PhysicalDeviceObject,
                            DeviceDescription,
                            NumberOfMapRegisters);
}

NTSTATUS
NTAPI
PiPagePathSetState(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN InPath)
{
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION IoStack;
    IO_STATUS_BLOCK Iosb;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
  
    PAGED_CODE();
    DPRINT("PiPagePathSetState: FileObject - %p, InPath - %X\n", FileObject, InPath);

    ObReferenceObject(FileObject);
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        DPRINT1("PiPagePathSetState: return STATUS_NO_MEMORY\n");
        return STATUS_NO_MEMORY;
    }

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;

    Irp->UserEvent = &Event;
    Irp->UserIosb = &Iosb;

    Irp->RequestorMode = KernelMode;
    Irp->Flags = IRP_SYNCHRONOUS_API;
    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->AssociatedIrp.SystemBuffer = NULL;

    IoStack = IoGetNextIrpStackLocation(Irp);

    IoStack->FileObject = FileObject;
    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_DEVICE_USAGE_NOTIFICATION;

    IoStack->Parameters.UsageNotification.InPath = InPath;
    IoStack->Parameters.UsageNotification.Type = DeviceUsageTypePaging;

    IoQueueThreadIrp(Irp);

    PpDevNodeLockTree(1);

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = Iosb.Status;
    }

    PpDevNodeUnlockTree(1);

    DPRINT("PiPagePathSetState: return Status - %X\n", Status);
    return Status;
}

NTSTATUS
NTAPI
PpPagePathAssign(
    _In_ PFILE_OBJECT FileObject)
{
    PAGED_CODE();
    return PiPagePathSetState(FileObject, TRUE);
}

PDEVICE_OBJECT
NTAPI
IopFindMountableDevice(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_OBJECT MountableDevice;

    for (MountableDevice = DeviceObject;
         MountableDevice != NULL;
         MountableDevice = MountableDevice->AttachedDevice)
    {
        if ((MountableDevice->Flags & DO_DEVICE_HAS_NAME) != 0 &&
            MountableDevice->Vpb)
        {
            break;
        }
    }

    return MountableDevice;
}

ULONG
NTAPI
IopIncrementDeviceObjectHandleCount(
    _Inout_ PDEVICE_OBJECT FileDeviceObject)
{
    return IopInterlockedIncrementUlong(LockQueueIoDatabaseLock,
                                        (PULONG)&FileDeviceObject->ReferenceCount);
}

PDEVICE_OBJECT
NTAPI
IopLockMountedDeviceForRemove(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorCode,
    _Out_ PLOCK_DEVICES_FOR_REMOVE MountedDevices)
{
    PDEVICE_OBJECT FileSystemDeviceObject = NULL;
    PVPB Vpb;
    KIRQL OldIrql;

    MountedDevices->RemoveDeviceObject = DeviceObject;
    MountedDevices->FileSystemDeviceObject = NULL;

    while (DeviceObject)
    {
        if (!DeviceObject->Vpb)
        {
            goto Next;
        }

        KeWaitForSingleObject(&DeviceObject->DeviceLock,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        IoAcquireVpbSpinLock(&OldIrql);

        Vpb = DeviceObject->Vpb;
        ASSERT(Vpb != NULL);

        if (MinorCode)
        {
            if (MinorCode == IRP_MN_QUERY_REMOVE_DEVICE ||
                MinorCode == IRP_MN_REMOVE_DEVICE ||
                MinorCode == IRP_MN_SURPRISE_REMOVAL)
            {
                Vpb->Flags |= VPB_REMOVE_PENDING;
            }
            else if (MinorCode == IRP_MN_CANCEL_REMOVE_DEVICE)
            {
                Vpb->Flags &= ~VPB_REMOVE_PENDING;
            }
        }

        if (Vpb->Flags & VPB_MOUNTED)
        {
            MountedDevices->RemoveDeviceObject = DeviceObject;
            FileSystemDeviceObject = Vpb->DeviceObject;
        }

        MountedDevices->FileSystemDeviceObject = FileSystemDeviceObject;
        IoReleaseVpbSpinLock(OldIrql);

        if (FileSystemDeviceObject)
        {
            IopIncrementDeviceObjectHandleCount(FileSystemDeviceObject);
        }

        KeSetEvent(&DeviceObject->DeviceLock, IO_NO_INCREMENT, FALSE);

        if (FileSystemDeviceObject)
        {
            return FileSystemDeviceObject;
        }
Next:
        OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);
        DeviceObject = DeviceObject->AttachedDevice;
        KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);
    }

    if (FileSystemDeviceObject)
    {
        return FileSystemDeviceObject;
    }

    return MountedDevices->RemoveDeviceObject;
}

VOID
NTAPI
IopUnlockMountedDeviceForRemove(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorCode,
    _In_ PLOCK_DEVICES_FOR_REMOVE MountedDevices)
{
    KIRQL OldIrql;

    while (DeviceObject)
    {
        if (DeviceObject->Vpb)
        {
            KeWaitForSingleObject(&DeviceObject->DeviceLock, Executive, KernelMode, FALSE, NULL);
            IoAcquireVpbSpinLock(&OldIrql);

            if (MinorCode == IRP_MN_REMOVE_DEVICE)
                DeviceObject->Vpb->Flags &= ~VPB_REMOVE_PENDING;

            IoReleaseVpbSpinLock(OldIrql);
            KeSetEvent(&DeviceObject->DeviceLock, IO_NO_INCREMENT, FALSE);
        }

        if (MountedDevices->RemoveDeviceObject == DeviceObject)
        {
            if (MountedDevices->FileSystemDeviceObject)
                IopDecrementDeviceObjectHandleCount(MountedDevices->FileSystemDeviceObject);

            break;
        }

        OldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);
        DeviceObject = DeviceObject->AttachedDevice;
        KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, OldIrql);
    }

    return;
}

NTSTATUS
NTAPI
IopRemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorCode)
{
    PDEVICE_OBJECT RemoveDeviceObject;
    LOCK_DEVICES_FOR_REMOVE MountedDevices;
    PDEVICE_NODE DeviceNode;
    IO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    BOOLEAN IsLocked = FALSE;

    PAGED_CODE();
    DPRINT("IopRemoveDevice: DeviceObject - %p, MinorCode - %X\n", DeviceObject, MinorCode);

    ASSERT(MinorCode == IRP_MN_QUERY_REMOVE_DEVICE ||
           MinorCode == IRP_MN_CANCEL_REMOVE_DEVICE ||
           MinorCode == IRP_MN_REMOVE_DEVICE ||
           MinorCode == IRP_MN_SURPRISE_REMOVAL ||
           MinorCode == IRP_MN_EJECT);

    DeviceNode = IopGetDeviceNode(DeviceObject);

    if (MinorCode == IRP_MN_REMOVE_DEVICE ||
        MinorCode == IRP_MN_QUERY_REMOVE_DEVICE)
    {
        IopUncacheInterfaceInformation(DeviceObject);
    }

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = MinorCode;

    if (IopFindMountableDevice(DeviceObject))
    {
        RemoveDeviceObject = IopLockMountedDeviceForRemove(DeviceObject,
                                                           MinorCode,
                                                           &MountedDevices);
        IsLocked = TRUE;
    }
    else
    {
        ASSERT(!(DeviceObject->Type == FILE_DEVICE_DISK ||
                 DeviceObject->Type == FILE_DEVICE_CD_ROM ||
                 DeviceObject->Type == FILE_DEVICE_TAPE ||
                 DeviceObject->Type == FILE_DEVICE_VIRTUAL_DISK));

        DPRINT1("IopRemoveDevice: Mass storage DeviceObject %p does not have VPB\n", DeviceObject);
        RemoveDeviceObject = DeviceObject;
    }

    if (MinorCode == IRP_MN_SURPRISE_REMOVAL ||
        MinorCode == IRP_MN_REMOVE_DEVICE)
    {
        if (DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE)
        {
            DeviceNode->UserFlags &= ~DNUF_NOT_DISABLEABLE;
            DPRINT1("IopRemoveDevice: FIXME IopDecDisableableDepends\n");
            //IopDecDisableableDepends(DeviceNode);
        }
    }

    Status = IopSynchronousCall(RemoveDeviceObject, &IoStack, NULL);
    DPRINT1("IopRemoveDevice: MinorCode - %X, Status - %X\n", MinorCode, Status);

    if (IsLocked)
    {
        IopUnlockMountedDeviceForRemove(DeviceObject, MinorCode, &MountedDevices);

        if ((MinorCode == IRP_MN_QUERY_REMOVE_DEVICE ||
             MinorCode == IRP_MN_SURPRISE_REMOVAL) && NT_SUCCESS(Status))
        {
            Status = IopInvalidateVolumesForDevice(DeviceObject);
        }
    }

    if (MinorCode != IRP_MN_REMOVE_DEVICE)
    {
        goto Exit;
    }

    /* MinorCode == IRP_MN_REMOVE_DEVICE */

    DeviceNode->Flags &= ~(DNF_REENUMERATE | DNF_LEGACY_DRIVER);

    if (DeviceNode->Parent)
    {
        goto Exit;
    }

    ASSERT(DeviceNode->PreviousParent);

    if (InterlockedDecrement((PLONG)&DeviceNode->PreviousParent->DeletedChildren))
    {
        goto Exit;
    }

    if (DeviceNode->PreviousParent->State != DeviceNodeDeletePendingCloses &&
        DeviceNode->PreviousParent->State != DeviceNodeRemovePendingCloses)
    {
        goto Exit;
    }

    IopNotifyPnpWhenChainDereferenced(&DeviceNode->PreviousParent->PhysicalDeviceObject,
                                      1,
                                      FALSE,
                                      FALSE,
                                      NULL);
Exit:

    DPRINT("IopRemoveDevice: return Status - %X\n", Status);
    return Status;
}

PDEVICE_OBJECT
NTAPI
IoFindDeviceThatFailedIrp(
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    ULONG StackCount;
    ULONG ix;

    if (NT_SUCCESS(Irp->IoStatus.Status))
    {
        return NULL;
    }

    StackCount = Irp->StackCount;

    IoStack = (PIO_STACK_LOCATION)((ULONG_PTR)Irp +
                                   sizeof(IRP) +
                                   ((StackCount - 1) * sizeof(IO_STACK_LOCATION)));

    for (ix = 0; ix < StackCount; ix++)
    {
        if (IoStack->Control & SL_ERROR_RETURNED)
        {
            return IoStack->DeviceObject;
        }

        IoStack--;
    }

    return NULL;
}

NTSTATUS
NTAPI
PiIrpQueryRemoveDeviceCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID Context)
{
    PDEVICE_OBJECT FailDeviceObject;
    PQUERY_REMOVE_DEVICE_CONTEXT QueryContext;

    QueryContext = Context;
    ASSERT(QueryContext);

    FailDeviceObject = IoFindDeviceThatFailedIrp(Irp);
    if (FailDeviceObject)
    {
        QueryContext->DriverObject = FailDeviceObject->DriverObject;
    }

    QueryContext->CompletionStatus = Irp->IoStatus.Status;

    IoFreeIrp(Irp);
    KeSetEvent(&QueryContext->Event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
PiIrpQueryRemoveDevice(
    _In_ PDEVICE_OBJECT TargetDevice,
    _Out_ PDRIVER_OBJECT * OutFailDriverObject)
{
    QUERY_REMOVE_DEVICE_CONTEXT QueryContext;
    LOCK_DEVICES_FOR_REMOVE MountedDevices;
    IO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    BOOLEAN IsLocked = FALSE;
  
    PAGED_CODE();
    DPRINT("PiIrpQueryRemoveDevice: TargetDevice - %p\n", TargetDevice);

    RtlZeroMemory(&IoStack, sizeof(IoStack));

    IoStack.MajorFunction = IRP_MJ_PNP;
    IoStack.MinorFunction = IRP_MN_QUERY_REMOVE_DEVICE;

    if (IopFindMountableDevice(TargetDevice))
    {
        TargetDevice = IopLockMountedDeviceForRemove(TargetDevice,
                                                     IRP_MN_QUERY_REMOVE_DEVICE,
                                                     &MountedDevices);
        IsLocked = TRUE;
    }
    else
    {
        ASSERT(!(TargetDevice->Type == FILE_DEVICE_DISK ||
                 TargetDevice->Type == FILE_DEVICE_CD_ROM ||
                 TargetDevice->Type == FILE_DEVICE_TAPE ||
                 TargetDevice->Type == FILE_DEVICE_VIRTUAL_DISK));

        DPRINT1("PiIrpQueryRemoveDevice: Mass storage %p does not have VPB\n", TargetDevice);
    }

    KeInitializeEvent(&QueryContext.Event, SynchronizationEvent, FALSE);

    QueryContext.DriverObject = NULL;
    QueryContext.CompletionStatus = STATUS_UNSUCCESSFUL;

    Status = PiAsynchronousCall(TargetDevice,
                                &IoStack,
                                PiIrpQueryRemoveDeviceCompletionRoutine,
                                &QueryContext);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&QueryContext.Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        Status = QueryContext.CompletionStatus;
    }

    if ( OutFailDriverObject )
    {
        *OutFailDriverObject = QueryContext.DriverObject;
    }

    DPRINT("PiIrpQueryRemoveDevice: Status - %X\n", Status);

    if (IsLocked)
    {
        IopUnlockMountedDeviceForRemove(TargetDevice,
                                        IRP_MN_QUERY_REMOVE_DEVICE,
                                        &MountedDevices);

        if (NT_SUCCESS(Status))
        {
            Status = IopInvalidateVolumesForDevice(TargetDevice);
        }
    }

    DPRINT("PiIrpQueryRemoveDevice: return Status - %X\n", Status);
    return Status;
}

/* EOF */

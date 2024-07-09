/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            ntoskrnl/io/debug.c
 * PURPOSE:         Useful functions for debugging IO and PNP managers
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

#include <ntoskrnl.h>
#include "pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

extern PDEVICE_NODE IopRootDeviceNode;

/* FUNCTIONS ******************************************************************/

/* CmResource */

/* PipDumpCmResourceDescriptor() displays information about a Cm Descriptor 

   DebugLevel: 0 - always dump
               1 - dump if not defined NDEBUG
*/
VOID
NTAPI
PipDumpCmResourceDescriptor(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ ULONG DebugLevel)
{
    PAGED_CODE();

    if (DebugLevel != 0)
    {
#ifdef NDEBUG
        return;
#endif
    }

    if (Descriptor == NULL)
    {
        DPRINT1("Dump CmDescriptor: Descriptor == NULL\n");
        return;
    }

    switch (Descriptor->Type)
    {
        case CmResourceTypePort:
            DPRINT1("[%p:%X:%X] IO:  Start %X:%X, Len %X\n", Descriptor, Descriptor->ShareDisposition, Descriptor->Flags, Descriptor->u.Port.Start.HighPart, Descriptor->u.Port.Start.LowPart, Descriptor->u.Port.Length);
            break;

        case CmResourceTypeInterrupt:
            DPRINT1("[%p:%X:%X] INT: Lev %X Vec %X Aff %IX\n", Descriptor, Descriptor->ShareDisposition, Descriptor->Flags, Descriptor->u.Interrupt.Level, Descriptor->u.Interrupt.Vector, Descriptor->u.Interrupt.Affinity);
            break;

        case CmResourceTypeMemory:
            DPRINT1("[%p:%X:%X] MEM: %X:%X Len %X\n", Descriptor, Descriptor->ShareDisposition, Descriptor->Flags, Descriptor->u.Memory.Start.HighPart, Descriptor->u.Memory.Start.LowPart, Descriptor->u.Memory.Length);
            break;

        case CmResourceTypeDma:
            DPRINT1("[%p:%X:%X] DMA: Channel %X Port %X\n", Descriptor, Descriptor->ShareDisposition, Descriptor->Flags, Descriptor->u.Dma.Channel, Descriptor->u.Dma.Port);
            break;

        case CmResourceTypeDeviceSpecific:
            DPRINT1("[%p:%X:%X] DAT: DataSize %X\n", Descriptor, Descriptor->ShareDisposition, Descriptor->Flags, Descriptor->u.DeviceSpecificData.DataSize);
            break;

        case CmResourceTypeBusNumber:
            DPRINT1("[%p:%X:%X] BUS: Start %X Len %X Reserv %X\n", Descriptor, Descriptor->ShareDisposition, Descriptor->Flags, Descriptor->u.BusNumber.Start, Descriptor->u.BusNumber.Length, Descriptor->u.BusNumber.Reserved);
            break;

        case CmResourceTypeDevicePrivate:
            DPRINT1("[%p:%X:%X] PVT: D[0] %X D[1] %X D[2] %X\n", Descriptor, Descriptor->ShareDisposition, Descriptor->Flags, Descriptor->u.DevicePrivate.Data[0], Descriptor->u.DevicePrivate.Data[1], Descriptor->u.DevicePrivate.Data[2]);
            break;

        default:
            DPRINT1("[%p] Unknown type %X\n", Descriptor, Descriptor->Type);
            break;
    }
}

/* PipGetNextCmPartialDescriptor() return poiner to next a Cm Descriptor */

PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
PipGetNextCmPartialDescriptor(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR NextDescriptor;

    /* Assume the descriptors are the fixed size ones */
    NextDescriptor = CmDescriptor + 1;

    /* But check if this is actually a variable-sized descriptor */
    if (CmDescriptor->Type == CmResourceTypeDeviceSpecific)
    {
        /* Add the size of the variable section as well */
        NextDescriptor = (PVOID)((ULONG_PTR)NextDescriptor +
                                 CmDescriptor->u.DeviceSpecificData.DataSize);
    }

    /* Now the correct pointer has been computed, return it */
    return NextDescriptor;
}

/* PipDumpCmResourceList() displays information about a Cm List 

   DebugLevel: 0 - always dump
               1 - dump if not defined NDEBUG
*/
VOID
NTAPI
PipDumpCmResourceList(
    _In_ PCM_RESOURCE_LIST CmResource,
    _In_ ULONG DebugLevel)
{
    PCM_FULL_RESOURCE_DESCRIPTOR FullList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG ix;
    ULONG jx;

    PAGED_CODE();

    if (DebugLevel != 0)
    {
#ifdef NDEBUG
        return;
#endif
    }

    DPRINT1("Dump CmList: CmResource %p\n", CmResource);

    if (CmResource == NULL)
    {
        DPRINT1("PipDumpCmResourceList: CmResource == NULL\n");
        return;
    }

    if (CmResource->Count == 0)
    {
        DPRINT1("PipDumpCmResourceList: CmResource->Count == 0\n");
        return;
    }

    DPRINT1("FullList Count %x\n", CmResource->Count);

    FullList = &CmResource->List[0];

    for (ix = 0; ix < CmResource->Count; ix++)
    {
        DPRINT1("List #%X Iface %X Bus #%X Ver.%X Rev.%X Count %X\n",
                ix,
                FullList->InterfaceType,
                FullList->BusNumber,
                FullList->PartialResourceList.Version,
                FullList->PartialResourceList.Revision,
                FullList->PartialResourceList.Count);

        Descriptor = FullList->PartialResourceList.PartialDescriptors;

        for (jx = 0; jx < FullList->PartialResourceList.Count; jx++)
        {
            PipDumpCmResourceDescriptor(Descriptor, DebugLevel);
            Descriptor = PipGetNextCmPartialDescriptor(Descriptor);
        }

        FullList = (PCM_FULL_RESOURCE_DESCRIPTOR)Descriptor;
    }
}

/* IoResource */

/* PipDumpIoResourceDescriptor() displays information about a Io Descriptor 

   DebugLevel: 0 - always dump
               1 - dump if not defined NDEBUG
*/
VOID
NTAPI
PipDumpIoResourceDescriptor(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ ULONG DebugLevel)
{
    PAGED_CODE();

    if (DebugLevel != 0)
    {
#ifdef NDEBUG
        return;
#endif
    }

    if (Descriptor == NULL)
    {
        DPRINT1("DumpResourceDescriptor: Descriptor == 0\n");
        return;
    }

    switch (Descriptor->Type)
    {
        case CmResourceTypeNull:
            DPRINT1("[%p:%X:%X] O: Len %X Align %X Min %I64X, Max %I64X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->u.Generic.Length, Descriptor->u.Generic.Alignment, Descriptor->u.Generic.MinimumAddress.QuadPart, Descriptor->u.Generic.MaximumAddress.QuadPart);
            break;

        case CmResourceTypePort:
            DPRINT1("[%p:%X:%X] IO: Min %X:%X, Max %X:%X, Align %X Len %X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->u.Port.MinimumAddress.HighPart, Descriptor->u.Port.MinimumAddress.LowPart, Descriptor->u.Port.MaximumAddress.HighPart, Descriptor->u.Port.MaximumAddress.LowPart, Descriptor->u.Port.Alignment, Descriptor->u.Port.Length);
            break;

        case CmResourceTypeInterrupt:
            DPRINT1("[%p:%X:%X] INT: Min %X Max %X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->u.Interrupt.MinimumVector, Descriptor->u.Interrupt.MaximumVector);
            break;

        case CmResourceTypeMemory:
            DPRINT1("[%p:%X:%X] MEM: Min %X:%X, Max %X:%X, Align %X Len %X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->u.Memory.MinimumAddress.HighPart, Descriptor->u.Memory.MinimumAddress.LowPart, Descriptor->u.Memory.MaximumAddress.HighPart, Descriptor->u.Memory.MaximumAddress.LowPart, Descriptor->u.Memory.Alignment, Descriptor->u.Memory.Length);
            break;

        case CmResourceTypeDma:
            DPRINT1("[%p:%X:%X] DMA: Min %X Max %X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->u.Dma.MinimumChannel, Descriptor->u.Dma.MaximumChannel);
            break;

        case CmResourceTypeBusNumber:
            DPRINT1("[%p:%X:%X] BUS: Min %X Max %X Len %X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->u.BusNumber.MinBusNumber, Descriptor->u.BusNumber.MaxBusNumber, Descriptor->u.BusNumber.Length);
            break;

        case CmResourceTypeConfigData:
            DPRINT1("[%p:%X:%X] CFG: Priority %X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->u.ConfigData.Priority);
            break;

        case CmResourceTypeDevicePrivate:
            DPRINT1("[%p:%X:%X] DAT: %X %X %X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->u.DevicePrivate.Data[0], Descriptor->u.DevicePrivate.Data[1], Descriptor->u.DevicePrivate.Data[2]);
            break;

        default:
            DPRINT1("[%p:%X:%X]. Unknown type %X\n", Descriptor, Descriptor->Option, Descriptor->ShareDisposition, Descriptor->Type);
            break;
    }
}

/* PipDumpResourceRequirementsList() displays information about a Io List 

   DebugLevel: 0 - always dump
               1 - dump if not defined NDEBUG
*/
VOID
NTAPI
PipDumpResourceRequirementsList(
    _In_ PIO_RESOURCE_REQUIREMENTS_LIST IoResource,
    _In_ ULONG DebugLevel)
{
    PIO_RESOURCE_LIST AltList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    ULONG ix;
    ULONG jx;

    PAGED_CODE();

    if (DebugLevel != 0)
    {
#ifdef NDEBUG
        return;
#endif
    }

    if (IoResource == NULL)
    {
        DPRINT1("PipDumpResourceRequirementsList: IoResource == 0\n");
        return;
    }

    DPRINT1("Dump RequirementsList: IoResource %p\n", IoResource);
    DPRINT1("Interface %X Bus %X Slot %X AlternativeLists %X\n",
            IoResource->InterfaceType,
            IoResource->BusNumber,
            IoResource->SlotNumber,
            IoResource->AlternativeLists);

    AltList = &IoResource->List[0];

    if (IoResource->AlternativeLists < 1)
    {
        DPRINT1("PipDumpResourceRequirementsList: AlternativeLists < 1\n");
        return;
    }

    for (ix = 0; ix < IoResource->AlternativeLists; ix++)
    {
        DPRINT1("AltList %p, AltList->Count %X\n", AltList, AltList->Count);

        for (jx = 0; jx < AltList->Count; jx++)
        {
            Descriptor = &AltList->Descriptors[jx];
            PipDumpIoResourceDescriptor(Descriptor, DebugLevel);
        }

        AltList = (PIO_RESOURCE_LIST)(AltList->Descriptors + AltList->Count);
        DPRINT1("End Descriptors %p\n", AltList);
    }
}

/* Gets the name of the device node state. */

PWSTR
NTAPI
PipGetDeviceNodeStateName(
    _In_ PNP_DEVNODE_STATE State)
{
    switch (State)
    {
        case DeviceNodeUnspecified:
            return L"DeviceNodeUnspecified";

        case DeviceNodeUninitialized:
            return L"DeviceNodeUninitialized";

        case DeviceNodeInitialized:
            return L"DeviceNodeInitialized";

        case DeviceNodeDriversAdded:
            return L"DeviceNodeDriversAdded";

        case DeviceNodeResourcesAssigned:
            return L"DeviceNodeResourcesAssigned";

        case DeviceNodeStartPending:
            return L"DeviceNodeStartPending";

        case DeviceNodeStartCompletion:
            return L"DeviceNodeStartCompletion";

        case DeviceNodeStartPostWork:
            return L"DeviceNodeStartPostWork";

        case DeviceNodeStarted:
            return L"DeviceNodeStarted";

        case DeviceNodeQueryStopped:
            return L"DeviceNodeQueryStopped";

        case DeviceNodeStopped:
            return L"DeviceNodeStopped";

        case DeviceNodeRestartCompletion:
            return L"DeviceNodeRestartCompletion";

        case DeviceNodeEnumeratePending:
            return L"DeviceNodeEnumeratePending";

        case DeviceNodeEnumerateCompletion:
            return L"DeviceNodeEnumerateCompletion";

        case DeviceNodeAwaitingQueuedDeletion:
            return L"DeviceNodeAwaitingQueuedDeletion";

        case DeviceNodeAwaitingQueuedRemoval:
            return L"DeviceNodeAwaitingQueuedRemoval";

        case DeviceNodeQueryRemoved:
            return L"DeviceNodeQueryRemoved";

        case DeviceNodeRemovePendingCloses:
            return L"DeviceNodeRemovePendingCloses";

        case DeviceNodeRemoved:
            return L"DeviceNodeRemoved";

        case DeviceNodeDeletePendingCloses:
            return L"DeviceNodeDeletePendingCloses";

        case DeviceNodeDeleted:
            return L"DeviceNodeDeleted";

        default:
            break;
    }

    if (State != MaxDeviceNodeState)
    {
        DPRINT1("PipGetDeviceNodeStateName: Unknown State %X\n", State);
    }

    return L"";
}

/* Dump list arbiters for the device node */

VOID
NTAPI
PipDumpArbiters(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG NodeLevel)
{
    DPRINT("Level %X DevNode %p for PDO %p\n", NodeLevel, DeviceNode, DeviceNode->PhysicalDeviceObject);

    /* Definitions needed for arbiter */
    UNIMPLEMENTED;
}

/* PipDumpDeviceNode() displays information about a device node

    Flags is bitmask parametr:
        1 - traversal of all children nodes
        2 - dump allocated resources and boot configuration resources
        4 - dump resources required
        8 - dump translated resources

    DebugLevel: 0 - always dump
                1 - dump if not defined NDEBUG
*/
VOID
NTAPI
PipDumpDeviceNode(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG NodeLevel,
    _In_ ULONG Flags,
    _In_ ULONG DebugLevel)
{
    PDEVICE_NODE ChildDeviceNode;

    if (DebugLevel != 0)
    {
#ifdef NDEBUG
        return;
#endif
    }

    DPRINT1("* Level %X DevNode %p for PDO %p\n", NodeLevel, DeviceNode, DeviceNode->PhysicalDeviceObject);
    DPRINT1("Instance    %wZ\n", &DeviceNode->InstancePath);
if (DeviceNode->ServiceName.Length)
    DPRINT1("Service     %wZ\n", &DeviceNode->ServiceName);
#if 0
    /* It is not used yet */
    DPRINT1("State       %X %S\n", DeviceNode->State, PipGetDeviceNodeStateName(DeviceNode->State));
    DPRINT1("Prev State  %X %S\n", DeviceNode->PreviousState, PipGetDeviceNodeStateName(DeviceNode->PreviousState));
#endif
if (DeviceNode->Problem)
    DPRINT1("Problem     %X\n", DeviceNode->Problem);
#if 0
    /* It is not implemeted yet */
    PipDumpArbiters(DeviceNode, NodeLevel);
#endif

    /* Allocated resources and  Boot configuration (reported by IRP_MN_QUERY_RESOURCES)*/
    if (Flags & PIP_DUMP_FL_RES_ALLOCATED)
    {
        if (DeviceNode->ResourceList)
        {
            DPRINT1("---------- ResourceList ----------\n");
            PipDumpCmResourceList(DeviceNode->ResourceList, DebugLevel);
        }

        if (DeviceNode->BootResources)
        {
            DPRINT1("---------- BootResources ----------\n");
            PipDumpCmResourceList(DeviceNode->BootResources, DebugLevel);
        }
    }

    /* Resources required (reported by IRP_MN_FILTER_RESOURCE_REQUIREMENTS) */
    if (Flags & PIP_DUMP_FL_RES_REQUIREMENTS)
    {
        if (DeviceNode->ResourceRequirements)
        {
            DPRINT1("---------- ResourceRequirements ----------\n");
            PipDumpResourceRequirementsList(DeviceNode->ResourceRequirements, DebugLevel);
        }
    }

    /* Translated resources (AllocatedResourcesTranslated)  */
    if (Flags & PIP_DUMP_FL_RES_TRANSLATED)
    {
        if (DeviceNode->ResourceListTranslated)
        {
            DPRINT1("---------- ResourceListTranslated ----------\n");
            PipDumpCmResourceList(DeviceNode->ResourceListTranslated, DebugLevel);
        }
    }

    /* Traversal of all children nodes */
    if (Flags & PIP_DUMP_FL_ALL_NODES)
    {
        for (ChildDeviceNode = DeviceNode->Child;
             ChildDeviceNode != NULL;
             ChildDeviceNode = ChildDeviceNode->Sibling)
        {
            /* Recursive call */
            PipDumpDeviceNode(ChildDeviceNode, (NodeLevel + 1), Flags, DebugLevel);
        }
    }
}

/* PipDumpDeviceNodes() displays information about a node(s) in the device tree

    If DeviceNode is NULL, then the dump starts at the beginning of the device tree.

    Flags is bitmask parametr:
        1 - traversal of all children nodes
        2 - dump allocated resources and boot configuration resources
        4 - dump resources required
        8 - dump translated resources

    DebugLevel: 0 - always dump
                1 - dump if not defined NDEBUG

    See also: Windows DDK -> General Driver Information ->
        Kernel-Mode Driver Architecture -> Design Guide -> Plug and Play ->
        Introduction to Plug and Play -> Hardware Resources
*/
VOID
NTAPI
PipDumpDeviceNodes(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG Flags,
    _In_ ULONG DebugLevel)
{
    if (DebugLevel != 0)
    {
#ifdef NDEBUG
        return;
#endif
    }

    DPRINT1("PipDumpDeviceNodes: [%X] Flags %X Level %X\n", DeviceNode, Flags, DebugLevel);

    if (DeviceNode == NULL)
        DeviceNode = IopRootDeviceNode;

    PipDumpDeviceNode(DeviceNode, 0, Flags, DebugLevel);
}

PWCHAR
NTAPI
IopGetBusName(
    _In_ INTERFACE_TYPE IfType)
{
    switch (IfType)
    {
       case Internal:
         return L"Internal";

       case Isa:
         return L"Isa";

       case Eisa:
         return L"Eisa";

       case MicroChannel:
         return L"MicroChannel";

       case TurboChannel:
         return L"TurboChannel";

       case PCIBus:
         return L"PCIBus";

       case VMEBus:
         return L"VMEBus";

       case NuBus:
         return L"NuBus";

       case PCMCIABus:
         return L"PCMCIABus";

       case CBus:
         return L"CBus";

       case MPIBus:
         return L"MPIBus";

       case MPSABus:
         return L"MPSABus";

       case ProcessorInternal:
         return L"ProcessorInternal";

       case PNPISABus:
         return L"PNPISABus";

       case PNPBus:
         return L"PNPBus";

       case Vmcs:
         return L"Other";

       case MaximumInterfaceType:
         return L"Root";

       default:
         DPRINT1("Invalid bus type: %d\n", IfType);
         return NULL;
    }
}

/* Displays information about a node in the device tree.
   See !IopDumpDeviceNode extension for WinDbg.
   IopDumpDeviceNode (Address [Flags] [Service])
   IopDumpDeviceNode (NULL,     1,      NULL)   - displays the entire device tree.
   IopDumpDeviceNode (1,        0,      NULL)   - displays all pending removals of device objects
   IopDumpDeviceNode (2,        0,      NULL)   - displays all pending ejects of device objects
*/

VOID
NTAPI
IopDumpDeviceNode(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG Flags,
    _In_ PUNICODE_STRING ServiceName)
{
    DPRINT("IopDumpDeviceNode: DeviceNode - %X, Flags - %X, ServiceName - %X\n",
           DeviceNode, Flags, ServiceName);

    if (DeviceNode == ULongToPtr(1) ||
        DeviceNode == ULongToPtr(2))
    {
        DPRINT("IopDumpDeviceNode: FIXME [pending removals] and [pending ejects] NOT IMPLEMENTED\n");
        ASSERT(FALSE);
        return;
    }

    if (ServiceName)
    {
        DPRINT("IopDumpDeviceNode: FIXME [ServiceName] - %zW NOT IMPLEMENTED\n", ServiceName);
        ASSERT(FALSE);
    }

    if (!DeviceNode)
        DeviceNode = IopRootDeviceNode;

    PipDumpDeviceNodes(DeviceNode, Flags, 0);
}

VOID
NTAPI
IopDumpReqDescriptor(
    _In_ PPNP_REQ_DESCRIPTOR Descriptor,
    _In_ ULONG Idx)
{
    PAGED_CODE();
    DPRINT("=== IopDumpReqDescriptor [%X]: %p ======\n", Idx, Descriptor);
    DPRINT("InterfaceType                - %X\n", Descriptor->InterfaceType);
    DPRINT("BusNumber                    - %X\n", Descriptor->BusNumber);
    DPRINT("IsArbitrated                 - %X\n", Descriptor->IsArbitrated);
    DPRINT("AltList                      - %p\n", Descriptor->AltList);
    DPRINT("DescNumber                   - %X\n", Descriptor->DescNumber);
    DPRINT("TranslatedReqDesc            - %p\n", Descriptor->TranslatedReqDesc);
    DPRINT("-------------------------------------\n");
    DPRINT("ReqEntry.Link.Flink          - %p\n", Descriptor->ReqEntry.Link.Flink);
    DPRINT("ReqEntry.Link.Blink          - %p\n", Descriptor->ReqEntry.Link.Blink);
    DPRINT("ReqEntry.Count               - %X\n", Descriptor->ReqEntry.Count);
    DPRINT("ReqEntry.IoDescriptor        - %p\n", Descriptor->ReqEntry.IoDescriptor);
    DPRINT("ReqEntry.PhysicalDevice      - %p\n", Descriptor->ReqEntry.PhysicalDevice);
    DPRINT("ReqEntry.AllocationType      - %X\n", Descriptor->ReqEntry.AllocationType);
    DPRINT("ReqEntry.Reserved1           - %X\n", Descriptor->ReqEntry.Reserved1);
    DPRINT("ReqEntry.Reserved2           - %X\n", Descriptor->ReqEntry.Reserved2);
    DPRINT("ReqEntry.InterfaceType       - %X\n", Descriptor->ReqEntry.InterfaceType);
    DPRINT("ReqEntry.SlotNumber          - %X\n", Descriptor->ReqEntry.SlotNumber);
    DPRINT("ReqEntry.BusNumber           - %X\n", Descriptor->ReqEntry.BusNumber);
    DPRINT("ReqEntry.pCmDescriptor       - %p\n", Descriptor->ReqEntry.pCmDescriptor);
    DPRINT("ReqEntry.Reserved3           - %X\n", Descriptor->ReqEntry.Reserved3);
    DPRINT("ReqEntry.Reserved4           - %X\n", Descriptor->ReqEntry.Reserved4);
    DPRINT("ReqEntry.CmDesc.Type         - %X\n", Descriptor->ReqEntry.CmDescriptor.Type);
    DPRINT("ReqEntry.CmDesc.Share        - %X\n", Descriptor->ReqEntry.CmDescriptor.ShareDisposition);
    DPRINT("ReqEntry.CmDesc.Flags        - %X\n", Descriptor->ReqEntry.CmDescriptor.Flags);
    DPRINT("ReqEntry.CmDesc.StartLo      - %X\n", Descriptor->ReqEntry.CmDescriptor.u.Generic.Start.LowPart);
    DPRINT("ReqEntry.CmDesc.StartHi      - %X\n", Descriptor->ReqEntry.CmDescriptor.u.Generic.Start.HighPart);
    DPRINT("ReqEntry.CmDesc.Length       - %X\n", Descriptor->ReqEntry.CmDescriptor.u.Generic.Length);
    DPRINT("-------------------------------------\n");
    DPRINT("DescriptorsCount             - %p\n", Descriptor->DescriptorsCount);
    DPRINT("DevicePrivateIoDesc          - %p\n", Descriptor->DevicePrivateIoDesc);
    DPRINT("(Arbiter|Translator)Entry    - %p\n", Descriptor->ArbiterEntry);
    DPRINT("=== IopDumpReqDescriptor end ===========\n");
    DPRINT("\n");
}

VOID
NTAPI
IopDumpAltList(
    _In_ PPNP_REQ_ALT_LIST AltList,
    _In_ ULONG Idx)
{
    ULONG ix;
    PAGED_CODE();

    DPRINT("=== IopDumpAltList [%X]: %p ======\n", Idx, AltList);
    DPRINT("ConfigPriority               - %X\n", AltList->ConfigPriority);
    DPRINT("Priority                     - %X\n", AltList->Priority);
    DPRINT("ReqList                      - %p\n", AltList->ReqList);
    DPRINT("ListNumber                   - %X\n", AltList->ListNumber);
    DPRINT("CountDescriptors             - %X\n", AltList->CountDescriptors);
    DPRINT("=== IopDumpAltList end =================\n");

    for (ix = 0; ix < AltList->CountDescriptors; ix++)
    {
        IopDumpReqDescriptor(AltList->ReqDescriptors[ix], ix);
    }
}

VOID
NTAPI
IopDumpResRequest(
    _In_ PPNP_RESOURCE_REQUEST ResRequest)
{
    ULONG ix;
    PAGED_CODE();

    DPRINT("=== IopDumpResRequest %p =======\n", ResRequest);
    DPRINT("PhysicalDevice               - %p\n", ResRequest->PhysicalDevice);
    DPRINT("Flags                        - %X\n", ResRequest->Flags);
    DPRINT("AllocationType               - %X\n", ResRequest->AllocationType);
    DPRINT("Priority                     - %X\n", ResRequest->Priority);
    DPRINT("Position                     - %X\n", ResRequest->Position);
    DPRINT("ResourceRequirements         - %p\n", ResRequest->ResourceRequirements);
    DPRINT("ReqList                      - %p\n", ResRequest->ReqList);
    DPRINT("ResourceAssignment           - %p\n", ResRequest->ResourceAssignment);
    DPRINT("TranslatedResourceAssignment - %p\n", ResRequest->TranslatedResourceAssignment);
    DPRINT("Status                       - %X\n", ResRequest->Status);
    DPRINT("=== IopDumpResRequest end ===========\n");

    if (!ResRequest->ReqList)
    {
        return;
    }

    DPRINT("InterfaceType                - %X\n", ResRequest->ReqList->InterfaceType);
    DPRINT("BusNumber                    - %X\n", ResRequest->ReqList->BusNumber);
    DPRINT("ResRequest                   - %p\n", ResRequest->ReqList->ResRequest);
    DPRINT("AltList1                     - %p\n", ResRequest->ReqList->AltList1);
    DPRINT("AltList2                     - %p\n", ResRequest->ReqList->AltList2);
    DPRINT("Count                        - %X\n", ResRequest->ReqList->Count);
    DPRINT("IopDumpResRequest: ===========================\n");

    for (ix = 0; ix < ResRequest->ReqList->Count; ix++)
    {
        IopDumpAltList(ResRequest->ReqList->AltLists[ix], ix);
    }
}

/* EOF */

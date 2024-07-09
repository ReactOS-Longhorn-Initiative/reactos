/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpnode.c
 * PURPOSE:         Device node handle code
 * PROGRAMMERS:     
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "../pnpio.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern KSPIN_LOCK IopPnPSpinLock;
extern ERESOURCE PiEngineLock;
extern ERESOURCE PiDeviceTreeLock;
extern ULONG IopMaxDeviceNodeLevel;
extern KSEMAPHORE PpRegistrySemaphore;
extern LIST_ENTRY IopLegacyBusInformationTable[MaximumInterfaceType];
extern PDEVICE_NODE IopRootDeviceNode;
extern PDEVICE_NODE IopInitHalDeviceNode;

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

PDEVICE_NODE
FASTCALL
IopGetDeviceNode(PDEVICE_OBJECT DeviceObject)
{
    return ((PEXTENDED_DEVOBJ_EXTENSION)DeviceObject->DeviceObjectExtension)->DeviceNode;
}

PDEVICE_NODE
NTAPI
PipAllocateDeviceNode(IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode;
    PAGED_CODE();

    /* Allocate it */
    DeviceNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_NODE), TAG_IO_DEVNODE);
    if (!DeviceNode)
    {
        DPRINT1("Allocate failed for PDO %p\n", PhysicalDeviceObject);
        return DeviceNode;
    }

    /* Statistics */
    InterlockedIncrement(&IopNumberDeviceNodes);

    /* Set it up */
    RtlZeroMemory(DeviceNode, sizeof(DEVICE_NODE));
    DeviceNode->InterfaceType = InterfaceTypeUndefined;
    DeviceNode->BusNumber = -1;
    DeviceNode->ChildInterfaceType = InterfaceTypeUndefined;
    DeviceNode->ChildBusNumber = -1;
    DeviceNode->ChildBusTypeIndex = -1;
//    KeInitializeEvent(&DeviceNode->EnumerationMutex, SynchronizationEvent, TRUE);
    InitializeListHead(&DeviceNode->DeviceArbiterList);
    InitializeListHead(&DeviceNode->DeviceTranslatorList);
    InitializeListHead(&DeviceNode->TargetDeviceNotify);
    InitializeListHead(&DeviceNode->DockInfo.ListEntry);
    InitializeListHead(&DeviceNode->PendedSetInterfaceState);
    InitializeListHead(&DeviceNode->LegacyBusListEntry);

    DeviceNode->State = DeviceNodeUninitialized;

    /* Check if there is a PDO */
    if (PhysicalDeviceObject)
    {
        /* Link it and remove the init flag */
        DeviceNode->PhysicalDeviceObject = PhysicalDeviceObject;
        ((PEXTENDED_DEVOBJ_EXTENSION)PhysicalDeviceObject->DeviceObjectExtension)->DeviceNode = DeviceNode;
        PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    DPRINT("Alloc DeviceNode %p for PDO %p\n", DeviceNode, PhysicalDeviceObject);

    /* Return the node */
    return DeviceNode;
}

VOID
NTAPI
PpDevNodeLockTree(
    _In_ ULONG LockLevel)
{
    ULONG SharedCount;
    ULONG ix;

    PAGED_CODE();
    DPRINT("PpDevNodeLockTree: LockLevel %X\n", LockLevel);

    KeEnterCriticalRegion();

    if (LockLevel == 0)
    {
        ExAcquireSharedWaitForExclusive(&PiDeviceTreeLock, TRUE);
        return;
    }

    if (LockLevel == 1)
    {
        ExAcquireResourceExclusiveLite(&PiEngineLock, TRUE);
        ExAcquireSharedWaitForExclusive(&PiDeviceTreeLock, TRUE);
        return;
    }

    if (LockLevel == 2)
    {
        ExAcquireResourceExclusiveLite(&PiEngineLock, TRUE);
        ExAcquireResourceExclusiveLite(&PiDeviceTreeLock, TRUE);
    }
    else if (LockLevel == 3)
    {
        ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
        ASSERT(ExIsResourceAcquiredSharedLite(&PiDeviceTreeLock) &&
              (!ExIsResourceAcquiredExclusiveLite(&PiDeviceTreeLock)));

        SharedCount = ExIsResourceAcquiredSharedLite(&PiDeviceTreeLock);

        for (ix = 0; ix < SharedCount; ix++)
        {
            ExReleaseResourceLite(&PiDeviceTreeLock);
        }

        for (ix = 0; ix < SharedCount; ix++)
        {
            ExAcquireResourceExclusiveLite(&PiDeviceTreeLock, TRUE);
        }
    }
    else
    {
        ASSERT(FALSE);
    }

    DPRINT("PpDevNodeLockTree: Locked\n");
}

VOID
NTAPI
PpDevNodeUnlockTree(
    _In_ ULONG LockLevel)
{
    PAGED_CODE();
    DPRINT("PpDevNodeUnlockTree: LockLevel %X\n", LockLevel);

    PpDevNodeAssertLockLevel(LockLevel);

    if (LockLevel == 0)
    {
        ExReleaseResourceLite(&PiDeviceTreeLock);
    }
    else if (LockLevel == 1 || LockLevel == 2)
    {
        ExReleaseResourceLite(&PiDeviceTreeLock);
        ExReleaseResourceLite(&PiEngineLock);
    }
    else if (LockLevel == 3)
    {
        ASSERT(ExIsResourceAcquiredExclusiveLite(&PiDeviceTreeLock));
        ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
        ExConvertExclusiveToSharedLite(&PiDeviceTreeLock);
    }
    else
    {
        ASSERT(FALSE);
    }

    KeLeaveCriticalRegion();
    DPRINT("PpDevNodeUnlockTree: UnLocked\n");
}

VOID
NTAPI
PipSetDevNodeState(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ PNP_DEVNODE_STATE NewState,
    _Out_ PNP_DEVNODE_STATE *OutPreviousState)
{
    PNP_DEVNODE_STATE PreviousState;
    KIRQL OldIrql;

    DPRINT("PipSetDevNodeState: DeviceNode - %p, NewState - %X\n",
           DeviceNode, NewState);

    ASSERT(NewState != DeviceNodeQueryStopped ||
           DeviceNode->State == DeviceNodeStarted);

    if (NewState == DeviceNodeDeleted ||
        NewState == DeviceNodeDeletePendingCloses)
    {
        ASSERT(!(DeviceNode->Flags & DNF_ENUMERATED));
    }

    KeAcquireSpinLock(&IopPnPSpinLock, &OldIrql);
    PreviousState = DeviceNode->State;

    if (PreviousState != NewState)
    {
        DeviceNode->State = NewState;
        DeviceNode->PreviousState = PreviousState;
        DeviceNode->StateHistory[DeviceNode->StateHistoryEntry] = PreviousState;
        DeviceNode->StateHistoryEntry = (DeviceNode->StateHistoryEntry + 1) % 20;
    }

    KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

    if (OutPreviousState)
    {
        DPRINT("PipSetDevNodeState: PreviousState - %X\n", PreviousState);
        *OutPreviousState = PreviousState;
    }

    if (NewState == DeviceNodeDeleted)
    {
        ASSERT(FALSE);
        //PpRemoveDeviceActionRequests(DeviceNode->PhysicalDeviceObject);
    }
}

VOID
NTAPI
PipRestoreDevNodeState(
    _In_ PDEVICE_NODE DeviceNode)
{
    PNP_DEVNODE_STATE PreviousState;
    PNP_DEVNODE_STATE CurrentState;
    KIRQL OldIrql;

    DPRINT("PipRestoreDevNodeState: DeviceNode %p\n", DeviceNode);

    ASSERT((DeviceNode->State == DeviceNodeQueryRemoved) ||
           (DeviceNode->State == DeviceNodeQueryStopped) ||
           (DeviceNode->State == DeviceNodeAwaitingQueuedRemoval) ||
           (DeviceNode->State == DeviceNodeAwaitingQueuedDeletion));

    KeAcquireSpinLock(&IopPnPSpinLock, &OldIrql);

    PreviousState = DeviceNode->PreviousState;
    CurrentState = DeviceNode->State;

    DeviceNode->StateHistory[DeviceNode->StateHistoryEntry] = CurrentState;
    DeviceNode->StateHistoryEntry = ((DeviceNode->StateHistoryEntry + 1) % 20);

    DeviceNode->State = PreviousState;
    DeviceNode->PreviousState = DeviceNodeUnspecified;

    KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

    DPRINT("PipRestoreDevNodeState: [%wZ] %X => %X\n", &DeviceNode->InstancePath, CurrentState, PreviousState);
}

VOID
NTAPI
PipSetDevNodeProblem(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ ULONG Problem)
{
    PAGED_CODE();

    ASSERT(Problem != 0);
    ASSERT(DeviceNode->State != DeviceNodeStarted);

    ASSERT(DeviceNode->State != DeviceNodeUninitialized ||
           ((DeviceNode->Flags & DNF_ENUMERATED) == 0) ||
           Problem == CM_PROB_INVALID_DATA);

    DeviceNode->Flags |= DNF_HAS_PROBLEM;
    DeviceNode->Problem = Problem;
}

VOID
NTAPI
PipClearDevNodeProblem(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();
    DeviceNode->Flags &= ~DNF_HAS_PROBLEM;
    DeviceNode->Problem = 0;
}

VOID
NTAPI
PpDevNodeInsertIntoTree(
    _In_ PDEVICE_NODE ParentNode,
    _In_ PDEVICE_NODE DeviceNode)
{
    KIRQL OldIrql;
    ULONG NodeLevel;

    DPRINT("PpDevNodeInsertIntoTree: %p, %p\n", ParentNode, DeviceNode);

    KeAcquireSpinLock(&IopPnPSpinLock, &OldIrql);

    NodeLevel = ParentNode->Level + 1;
    DeviceNode->Level = NodeLevel;

    IopMaxDeviceNodeLevel = max(IopMaxDeviceNodeLevel, NodeLevel);

    DeviceNode->Parent = ParentNode;

    if (ParentNode->LastChild)
    {
        ASSERT(ParentNode->LastChild->Sibling == NULL);
        ParentNode->LastChild->Sibling = DeviceNode;
    }
    else
    {
        ASSERT(ParentNode->Child == NULL);
        ParentNode->Child = DeviceNode;
    }

    ParentNode->LastChild = DeviceNode;

    KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);
    //IoDeviceNodeTreeSequence++;
}

VOID
NTAPI
PpDevNodeAssertLockLevel(
    _In_ LONG LockLevel)
{
    PAGED_CODE();

    switch (LockLevel)
    {
        case 0:
            ASSERT(ExIsResourceAcquiredSharedLite(&PiDeviceTreeLock));
            break;

        case 1:
            ASSERT(ExIsResourceAcquiredSharedLite(&PiDeviceTreeLock));
            ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
            break;

        case 2:
        case 3:
            ASSERT(ExIsResourceAcquiredExclusiveLite(&PiDeviceTreeLock));
            ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
            break;

        default:
            ASSERT(FALSE);
            break;
    }
}

VOID
NTAPI
PiHotSwapGetDetachableNode(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PDEVICE_NODE * OutDeviceNode)
{
    PDEVICE_NODE CurrentNode;
    DEVICE_CAPABILITIES_FLAGS CapsFlags;

    PAGED_CODE();
    DPRINT("PiHotSwapGetDetachableNode: DeviceNode %p\n", DeviceNode);

    PpDevNodeAssertLockLevel(0);

    for (CurrentNode = DeviceNode;
         CurrentNode;
         CurrentNode = CurrentNode->Parent)
    {
        CapsFlags.AsULONG = CurrentNode->CapabilityFlags;

        if (CapsFlags.EjectSupported || CapsFlags.Removable)
        {
            DPRINT("PiHotSwapGetDetachableNode: CurrentNode %p\n", CurrentNode);
            break;
        }
    }

    *OutDeviceNode = CurrentNode;
}

VOID NTAPI
PiHotSwapGetDefaultBusRemovalPolicy(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ DEVICE_REMOVAL_POLICY * OutPolicy)
{
    DEVICE_REMOVAL_POLICY Policy;
    PDEVICE_NODE ParentNode;

    PAGED_CODE();
    DPRINT("PiHotSwapGetDefaultBusRemovalPolicy: DeviceNode %p\n", DeviceNode);

    PpDevNodeAssertLockLevel(1);

    if ((DeviceNode->InstancePath.Length > (wcslen(L"USB\\") * sizeof(WCHAR)) &&
         !_wcsnicmp(DeviceNode->InstancePath.Buffer, L"USB\\", wcslen(L"USB\\"))) ||
        (DeviceNode->InstancePath.Length > (wcslen(L"1394\\") * sizeof(WCHAR)) &&
         !_wcsnicmp(DeviceNode->InstancePath.Buffer, L"1394\\", wcslen(L"1394\\"))) ||
        (DeviceNode->InstancePath.Length > (wcslen(L"SBP2\\") * sizeof(WCHAR)) &&
         !_wcsnicmp(DeviceNode->InstancePath.Buffer, L"SBP2\\", wcslen(L"SBP2\\"))) ||
        (DeviceNode->InstancePath.Length > (wcslen(L"PCMCIA\\") * sizeof(WCHAR)) &&
         !_wcsnicmp(DeviceNode->InstancePath.Buffer, L"PCMCIA\\", wcslen(L"PCMCIA\\"))))
    {
        DPRINT("PiHotSwapGetDefaultBusRemovalPolicy: *OutPolicy - 5\n");
        *OutPolicy = 5;
        return;
    }

    ParentNode = DeviceNode->Parent;

    if (((DeviceNode->InstancePath.Length > (wcslen(L"PCI\\") * sizeof(WCHAR)) && 
          !_wcsnicmp(DeviceNode->InstancePath.Buffer, L"PCI\\", wcslen(L"PCI\\"))) &&
         (ParentNode->ServiceName.Length == (wcslen(L"PCMCIA") * sizeof(WCHAR)) &&
          !_wcsicmp(ParentNode->ServiceName.Buffer, L"PCMCIA"))))
    {
        DPRINT("PiHotSwapGetDefaultBusRemovalPolicy: *OutPolicy - 5\n");
        Policy = 5;
    }
    else
    {
        DPRINT("PiHotSwapGetDefaultBusRemovalPolicy: *OutPolicy - 4\n");
        Policy = 4;
    }

    DPRINT("PiHotSwapGetDefaultBusRemovalPolicy: Policy - %X\n", Policy);
    *OutPolicy = Policy;
}

VOID
NTAPI
PpHotSwapUpdateRemovalPolicy(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVICE_NODE DetachableNode;
    PDEVICE_OBJECT Pdo;
    DEVICE_REMOVAL_POLICY Policy;
    ULONG PolicySize;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("PpHotSwapUpdateRemovalPolicy: DeviceNode - %p\n", DeviceNode);

    PpDevNodeAssertLockLevel(1);

    PiHotSwapGetDetachableNode(DeviceNode, &DetachableNode);

    if (!DetachableNode)
    {
        DeviceNode->RemovalPolicy = RemovalPolicyExpectNoRemoval;
        DeviceNode->HardwareRemovalPolicy = RemovalPolicyExpectNoRemoval;

        DPRINT("PpHotSwapUpdateRemovalPolicy: RemovalPolicy - %X, HardwareRemovalPolicy - %X\n",
               DeviceNode->RemovalPolicy, DeviceNode->HardwareRemovalPolicy);

        return;
    }

    Pdo = DeviceNode->PhysicalDeviceObject;

    if ((Pdo->Characteristics & 0x300) == 0x200) // (FILE_DEVICE_SECURE_OPEN | ???)
    {
        Policy = RemovalPolicyExpectOrderlyRemoval;
    }
    else if ((Pdo->Characteristics & 0x300) == 0x300)
    {
        Policy = RemovalPolicyExpectSurpriseRemoval;
    }
    else if (DeviceNode == DetachableNode)
    {
        PiHotSwapGetDefaultBusRemovalPolicy(DeviceNode, &Policy);
    }
    else
    {
        Policy = 6; //?
    }

    if (DeviceNode != DetachableNode &&
        Policy > DeviceNode->Parent->RemovalPolicy)
    {
        Policy = DeviceNode->Parent->RemovalPolicy;
    }

    DeviceNode->RemovalPolicy = Policy;
    DeviceNode->HardwareRemovalPolicy = Policy;

    PolicySize = sizeof(Policy);

    Status = PiGetDeviceRegistryProperty(Pdo,
                                         REG_DWORD,
                                         L"RemovalPolicy",
                                         NULL,
                                         &Policy,
                                         &PolicySize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("PpHotSwapUpdateRemovalPolicy: Status - %X\n", Status);
        return;
    }

    if (Policy == RemovalPolicyExpectOrderlyRemoval ||
        Policy == RemovalPolicyExpectSurpriseRemoval)
    {
        DeviceNode->RemovalPolicy = Policy;
    }

    DPRINT("PpHotSwapUpdateRemovalPolicy: RemovalPolicy - %X, HardwareRemovalPolicy - %X\n",
           DeviceNode->RemovalPolicy, DeviceNode->HardwareRemovalPolicy);
}

VOID
NTAPI
PpHotSwapGetDevnodeRemovalPolicy(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN Type,
    _Out_ PDEVICE_REMOVAL_POLICY OutPolicy)
{
    PDEVICE_NODE DetachableNode;
    DEVICE_REMOVAL_POLICY Policy;
    ULONG RemovalPolicy;

    PAGED_CODE();
    DPRINT("PpHotSwapGetDevnodeRemovalPolicy: DeviceNode %p\n", DeviceNode);

    PpDevNodeLockTree(0);

    if (Type)
        RemovalPolicy = DeviceNode->RemovalPolicy;
    else
        RemovalPolicy = DeviceNode->HardwareRemovalPolicy;

    Policy = RemovalPolicy;

    switch (RemovalPolicy)
    {
        case 0:
        {
            PiHotSwapGetDetachableNode(DeviceNode, &DetachableNode);

            if (!DetachableNode)
                Policy = RemovalPolicyExpectNoRemoval;
            else if (DetachableNode->CapabilityFlags & 8)
                Policy = RemovalPolicyExpectOrderlyRemoval;
            else if (DetachableNode->CapabilityFlags & 0x10)
                Policy = RemovalPolicyExpectSurpriseRemoval;
            else
                ASSERT(FALSE); // IoDbgBreakPointEx();

            break;
        }
        case 1:
        case 2:
        case 3:
            break;

        case 4:
            Policy = RemovalPolicyExpectOrderlyRemoval;
            break;

        case 5:
            Policy = RemovalPolicyExpectSurpriseRemoval;
            break;

        case 6:
            Policy = RemovalPolicyExpectOrderlyRemoval;
            break;

        default:
            ASSERT(FALSE); // IoDbgBreakPointEx();
            break;
    }

    PpDevNodeUnlockTree(0);
    DPRINT("PpHotSwapGetDevnodeRemovalPolicy: Policy %X\n", Policy);

    *OutPolicy = Policy;
}

VOID
NTAPI
PpHotSwapInitRemovalPolicy(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    DeviceNode->RemovalPolicy = 0;
    DeviceNode->HardwareRemovalPolicy = 0;
}

VOID
NTAPI
IopInsertLegacyBusDeviceNode(
    _In_ PDEVICE_NODE LegacyDeviceNode,
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber)
{
    PLIST_ENTRY Head;
    PLIST_ENTRY Entry;
    PDEVICE_NODE deviceNode;
    ULONG busNumber;
  
    PAGED_CODE();
    DPRINT("IopInsertLegacyBusDeviceNode: LegacyDeviceNode - %p, InterfaceType - %X, BusNumber - %X\n",
           LegacyDeviceNode, InterfaceType, BusNumber);

    ASSERT (InterfaceType < MaximumInterfaceType &&
            InterfaceType > InterfaceTypeUndefined);

    if (InterfaceType >= MaximumInterfaceType ||
        InterfaceType <= InterfaceTypeUndefined ||
        InterfaceType == PNPBus)
    {
        return;
    }

    if (InterfaceType == Eisa)
        InterfaceType = Isa;

    KeEnterCriticalRegion();
    KeWaitForSingleObject(&PpRegistrySemaphore,
                          DelayExecution,
                          KernelMode,
                          FALSE,
                          NULL);

    Head = &IopLegacyBusInformationTable[InterfaceType];

    for (Entry = Head->Flink; ; Entry = Entry->Flink)
    {
        if (Entry == Head)
        {
            DPRINT("IopInsertLegacyBusDeviceNode: Inserting: InstancePath - %wZ, Interface - %X,  BusNumber - %X\n",
                   &LegacyDeviceNode->InstancePath, InterfaceType, BusNumber);

            InsertTailList(Entry, &LegacyDeviceNode->LegacyBusListEntry);
            break;
        }

        deviceNode = CONTAINING_RECORD(Entry, DEVICE_NODE, LegacyBusListEntry);
        busNumber = deviceNode->BusNumber;

        if (busNumber == BusNumber)
        {
            if (deviceNode != LegacyDeviceNode)
            {
                DPRINT("IopInsertLegacyBusDeviceNode: Identical deviceNode - %wZ and LegacyDeviceNode - %\n",
                       &deviceNode->InstancePath, &LegacyDeviceNode->InstancePath);
            }

            break;
        }

        if (busNumber > BusNumber)
        {
            DPRINT("IopInsertLegacyBusDeviceNode: Inserting: InstancePath - %wZ, Interface - %X,  BusNumber - %X\n",
                   &LegacyDeviceNode->InstancePath, InterfaceType, BusNumber);

            InsertTailList(Entry, &LegacyDeviceNode->LegacyBusListEntry);
            break;
        }
    }

    KeReleaseSemaphore(&PpRegistrySemaphore, IO_NO_INCREMENT, 1, FALSE);
    KeLeaveCriticalRegion();
}

BOOLEAN
NTAPI
PipIsDevNodeDNStarted(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();

    if (DeviceNode->State == DeviceNodeUnspecified)
    {
        ASSERT(FALSE); // IoDbgBreakPointEx();
        return FALSE;
    }

    if (DeviceNode->State == DeviceNodeUninitialized ||
        DeviceNode->State == DeviceNodeInitialized ||
        DeviceNode->State == DeviceNodeDriversAdded ||
        DeviceNode->State == DeviceNodeResourcesAssigned)
    {
        return FALSE;
    }

    if (DeviceNode->State == DeviceNodeStartPending ||
        DeviceNode->State == DeviceNodeStartCompletion ||
        DeviceNode->State == DeviceNodeStartPostWork ||
        DeviceNode->State == DeviceNodeStarted ||
        DeviceNode->State == DeviceNodeQueryStopped ||
        DeviceNode->State == DeviceNodeStopped ||
        DeviceNode->State == DeviceNodeRestartCompletion ||
        DeviceNode->State == DeviceNodeEnumeratePending ||
        DeviceNode->State == DeviceNodeEnumerateCompletion)
    {
        return TRUE;
    }

    if (DeviceNode->State == DeviceNodeAwaitingQueuedDeletion ||
        DeviceNode->State == DeviceNodeAwaitingQueuedRemoval ||
        DeviceNode->State == DeviceNodeQueryRemoved ||
        DeviceNode->State == DeviceNodeRemovePendingCloses ||
        DeviceNode->State == DeviceNodeRemoved ||
        DeviceNode->State == DeviceNodeDeletePendingCloses ||
        DeviceNode->State == DeviceNodeDeleted)
    {
        return FALSE;
    }

    DPRINT1("PipIsDevNodeDNStarted: Unknown State %X\n", DeviceNode->State);
    ASSERT(FALSE); // IoDbgBreakPointEx();

    return FALSE;
}

BOOLEAN
NTAPI
PipAreDriversLoadedWorker(
    _In_ PNP_DEVNODE_STATE State,
    _In_ PNP_DEVNODE_STATE PreviousState)
{
    PAGED_CODE();

    while (TRUE)
    {
        switch (State)
        {
            case DeviceNodeAwaitingQueuedDeletion:
                State = PreviousState;
                PreviousState = DeviceNodeUnspecified;
                continue;

            case DeviceNodeDriversAdded:
            case DeviceNodeResourcesAssigned:
            case DeviceNodeStartCompletion:
            case DeviceNodeStartPostWork:
            case DeviceNodeStarted:
            case DeviceNodeQueryStopped:
            case DeviceNodeStopped:
            case DeviceNodeRestartCompletion:
            case DeviceNodeEnumerateCompletion:
            case DeviceNodeAwaitingQueuedRemoval:
            case DeviceNodeQueryRemoved:
            case DeviceNodeRemovePendingCloses:
            case DeviceNodeDeletePendingCloses:
                return TRUE;

            case DeviceNodeUninitialized:
            case DeviceNodeInitialized:
            case DeviceNodeRemoved:
            case DeviceNodeDeleted:
                return FALSE;

            default:
              ASSERT(FALSE); // IoDbgBreakPointEx();
              break;
        }

        break;
    }

    return FALSE;
}

BOOLEAN
NTAPI
PipAreDriversLoaded(
    _In_ PDEVICE_NODE DeviceNode)
{
    PAGED_CODE();
    return PipAreDriversLoadedWorker(DeviceNode->State,
                                     DeviceNode->PreviousState);
}

VOID
NTAPI
IopDestroyDeviceNode(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT dbgDeviceObject;
    PLIST_ENTRY Entry;

    PAGED_CODE();

    if (!DeviceNode)
    {
        DPRINT("IopDestroyDeviceNode: DeviceNode is NULL\n");
        return;
    }

    Pdo = DeviceNode->PhysicalDeviceObject;
    DPRINT1("IopDestroyDeviceNode: DeviceNode %p, Pdo %p\n", DeviceNode, Pdo);

    if ((Pdo->Flags & DO_BUS_ENUMERATED_DEVICE) && DeviceNode->Parent)
    {
        DPRINT1("KeBugCheckEx(PNP_DETECTED_FATAL_ERROR)\n");
        ASSERT(FALSE);//IoDbgBreakPointEx();
        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 5, (ULONG_PTR)DeviceNode->PhysicalDeviceObject, 0, 0);
    }

    if (DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE)
    {
        IopLegacyResourceAllocation(ArbiterRequestUndefined, IopRootDriverObject, Pdo, NULL, NULL);
        return;
    }

    ASSERT(DeviceNode->Child == NULL &&
           DeviceNode->Sibling == NULL &&
           DeviceNode->LastChild == NULL);

    ASSERT(DeviceNode->DockInfo.SerialNumber == NULL &&
          IsListEmpty(&DeviceNode->DockInfo.ListEntry));

    ASSERT(DeviceNode->Parent == NULL);

#if DBG
    if (DeviceNode->PrevCmResource)
        ExFreePool(DeviceNode->PrevCmResource);

    if (DeviceNode->DbgParam2)
        ExFreePool(DeviceNode->DbgParam2);
#endif

    ASSERT((DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE) == 0);
    ASSERT(DeviceNode->DisableableDepends == 0);

    if (DeviceNode->InstancePath.Length)
    {
        dbgDeviceObject = IopDeviceObjectFromDeviceInstance(&DeviceNode->InstancePath);
        if (dbgDeviceObject)
        {
            ASSERT(dbgDeviceObject != DeviceNode->PhysicalDeviceObject);
            ObDereferenceObject(dbgDeviceObject);
        }
    }

    if (DeviceNode->DuplicatePDO)
        ObDereferenceObject(DeviceNode->DuplicatePDO);

    if (DeviceNode->ServiceName.Length)
        ExFreePool(DeviceNode->ServiceName.Buffer);

    if (DeviceNode->InstancePath.Length)
        ExFreePool(DeviceNode->InstancePath.Buffer);

    if (DeviceNode->ResourceRequirements)
        ExFreePool(DeviceNode->ResourceRequirements);

    IopUncacheInterfaceInformation(DeviceNode->PhysicalDeviceObject);

    while (!IsListEmpty(&DeviceNode->PendedSetInterfaceState))
    {
        Entry = RemoveHeadList(&DeviceNode->PendedSetInterfaceState);

        DPRINT1("IopDestroyDeviceNode: FIXME! DeviceNode %p, Entry %p\n", DeviceNode, Entry);
        ASSERT(FALSE);//IoDbgBreakPointEx();

        //ExFreePool(?); // ? DEVNODE_INTERFACE_STATE.SymbolicLinkName.Buffer
        ExFreePool(Entry);
    }

    ((PEXTENDED_DEVOBJ_EXTENSION)(DeviceNode->PhysicalDeviceObject->DeviceObjectExtension))->DeviceNode = NULL;

    ExFreePoolWithTag(DeviceNode, TAG_IO_DEVNODE);

    InterlockedDecrement(&IopNumberDeviceNodes);
}

VOID
NTAPI
IopMarkHalDeviceNode(VOID)
{
    PDEVICE_NODE DeviceNode;

    for (DeviceNode = IopRootDeviceNode->Child;
         DeviceNode;
         DeviceNode = DeviceNode->Sibling)
    {
        if ((DeviceNode->State == DeviceNodeStarted ||
             DeviceNode->State == DeviceNodeStartPostWork) &&
            !(DeviceNode->Flags & DNF_LEGACY_DRIVER))
        {
            IopInitHalDeviceNode = DeviceNode;
            DeviceNode->Flags |= DNF_HAL_NODE;
            return;
        }
    }
}

BOOLEAN
NTAPI
PipIsProblemReadonly(
    _In_ ULONG Problem)
{
    BOOLEAN Result;

    PAGED_CODE();

    switch (Problem)
    {
        case CM_PROB_NOT_CONFIGURED:
        case CM_PROB_FAILED_START:
        case CM_PROB_NEED_RESTART:
        case CM_PROB_REINSTALL:
        case CM_PROB_REGISTRY:
        case CM_PROB_WILL_BE_REMOVED:
        case CM_PROB_DISABLED:
        case CM_PROB_FAILED_INSTALL:
        case CM_PROB_FAILED_ADD:
        case CM_PROB_DISABLED_SERVICE:
        case CM_PROB_FAILED_DRIVER_ENTRY:
        case CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD:
        case CM_PROB_DRIVER_FAILED_LOAD:
        case CM_PROB_DRIVER_SERVICE_KEY_INVALID:
        case CM_PROB_LEGACY_SERVICE_NO_DEVICES:
        case CM_PROB_FAILED_POST_START:
        case CM_PROB_HALTED:
        case CM_PROB_DRIVER_BLOCKED:
            Result = FALSE;
            break;

        case CM_PROB_OUT_OF_MEMORY:
        case CM_PROB_INVALID_DATA:
        case CM_PROB_NORMAL_CONFLICT:
        case CM_PROB_PARTIAL_LOG_CONF:
        case CM_PROB_DEVICE_NOT_THERE:
        case CM_PROB_HARDWARE_DISABLED:
        case CM_PROB_TRANSLATION_FAILED:
        case CM_PROB_NO_SOFTCONFIG:
        case CM_PROB_BIOS_TABLE:
        case CM_PROB_IRQ_TRANSLATION_FAILED:
        case CM_PROB_DUPLICATE_DEVICE:
        case CM_PROB_SYSTEM_SHUTDOWN:
        case CM_PROB_HELD_FOR_EJECT:
        case CM_PROB_REGISTRY_TOO_LARGE:
        case CM_PROB_SETPROPERTIES_FAILED:
            Result = TRUE;
            break;

        default:
            DPRINT1("PipIsProblemReadonly: Problem %X\n", Problem);
            ASSERT(FALSE);
            Result = TRUE;
            break;
    }

    return Result;
}

VOID
NTAPI
PpDevNodeRemoveFromTree(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVICE_NODE* pNode;
    KIRQL OldIrql;

    DPRINT1("PpDevNodeRemoveFromTree: DeviceNode %p\n", DeviceNode);

    KeAcquireSpinLock(&IopPnPSpinLock, &OldIrql);

    DPRINT1("DeviceNode->Parent %p\n", DeviceNode->Parent);
    DPRINT1("DeviceNode->Parent->Child %p\n", DeviceNode->Parent->Child);

    pNode = &DeviceNode->Parent->Child;
    DPRINT1("PpDevNodeRemoveFromTree: pNode %p\n", pNode);

    while (*pNode != DeviceNode)
    {
        pNode = (PDEVICE_NODE *)*pNode;
        DPRINT1("PpDevNodeRemoveFromTree: pNode %p\n", pNode);
    }

    DPRINT1("DeviceNode->Sibling %p\n", DeviceNode->Sibling);
    *pNode = DeviceNode->Sibling;
    DPRINT1("DeviceNode->Parent->Child %p\n", DeviceNode->Parent->Child);

    if (DeviceNode->Parent->Child)
    {
        while (*pNode)
        {
            pNode = (PDEVICE_NODE *)*pNode;
            DPRINT1("PpDevNodeRemoveFromTree: pNode %p\n", pNode);
        }

        DeviceNode->Parent->LastChild = (PDEVICE_NODE)pNode;
    }
    else
    {
        DeviceNode->Parent->LastChild = NULL;
    }

    KeReleaseSpinLock(&IopPnPSpinLock, OldIrql);

    DPRINT1("FIXME RemoveEntryList(&DeviceNode->LegacyBusListEntry)\n");
    //RemoveEntryList(&DeviceNode->LegacyBusListEntry);

    DPRINT1("FIXME IopOrphanNotification\n");
    //IopOrphanNotification(DeviceNode);

    DeviceNode->PreviousParent = DeviceNode->Parent;

    DeviceNode->Parent = NULL;
    DeviceNode->Child = NULL;
    DeviceNode->Sibling = NULL;
    DeviceNode->LastChild = NULL;

    DPRINT1("DeviceNode->PreviousParent %p\n", DeviceNode->PreviousParent);
}

/* EOF */

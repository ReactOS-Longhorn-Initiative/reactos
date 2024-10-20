/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ex/lookas.c
* PURPOSE:         Lookaside Lists
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY ExpNonPagedLookasideListHead;
KSPIN_LOCK ExpNonPagedLookasideListLock;
LIST_ENTRY ExpPagedLookasideListHead;
KSPIN_LOCK ExpPagedLookasideListLock;
LIST_ENTRY ExSystemLookasideListHead;
LIST_ENTRY ExPoolLookasideListHead;
GENERAL_LOOKASIDE ExpSmallNPagedPoolLookasideLists[NUMBER_POOL_LOOKASIDE_LISTS];
GENERAL_LOOKASIDE ExpSmallPagedPoolLookasideLists[NUMBER_POOL_LOOKASIDE_LISTS];

/* PRIVATE FUNCTIONS *********************************************************/

CODE_SEG("INIT")
VOID
NTAPI
ExInitializeSystemLookasideList(IN PGENERAL_LOOKASIDE List,
                                IN POOL_TYPE Type,
                                IN ULONG Size,
                                IN ULONG Tag,
                                IN USHORT MaximumDepth,
                                IN PLIST_ENTRY ListHead)
{
    /* Initialize the list */
    List->Tag = Tag;
    List->Type = Type;
    List->Size = Size;
    InsertHeadList(ListHead, &List->ListEntry);
    List->MaximumDepth = MaximumDepth;
    List->Depth = 2;
    List->Allocate = ExAllocatePoolWithTag;
    List->Free = ExFreePool;
    InitializeSListHead(&List->ListHead);
    List->TotalAllocates = 0;
    List->AllocateHits = 0;
    List->TotalFrees = 0;
    List->FreeHits = 0;
    List->LastTotalAllocates = 0;
    List->LastAllocateHits = 0;
}

CODE_SEG("INIT")
VOID
NTAPI
ExInitPoolLookasidePointers(VOID)
{
    ULONG i;
    PKPRCB Prcb = KeGetCurrentPrcb();
    PGENERAL_LOOKASIDE Entry;

    /* Loop for all pool lists */
    for (i = 0; i < NUMBER_POOL_LOOKASIDE_LISTS; i++)
    {
        /* Initialize the non-paged list */
        Entry = &ExpSmallNPagedPoolLookasideLists[i];
        InitializeSListHead(&Entry->ListHead);

        /* Bind to PRCB */
        Prcb->PPNPagedLookasideList[i].P = Entry;
        Prcb->PPNPagedLookasideList[i].L = Entry;

        /* Initialize the paged list */
        Entry = &ExpSmallPagedPoolLookasideLists[i];
        InitializeSListHead(&Entry->ListHead);

        /* Bind to PRCB */
        Prcb->PPPagedLookasideList[i].P = Entry;
        Prcb->PPPagedLookasideList[i].L = Entry;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
ExpInitLookasideLists(VOID)
{
    ULONG i;

    /* Initialize locks and lists */
    InitializeListHead(&ExpNonPagedLookasideListHead);
    InitializeListHead(&ExpPagedLookasideListHead);
    InitializeListHead(&ExSystemLookasideListHead);
    InitializeListHead(&ExPoolLookasideListHead);
    KeInitializeSpinLock(&ExpNonPagedLookasideListLock);
    KeInitializeSpinLock(&ExpPagedLookasideListLock);

    /* Initialize the system lookaside lists */
    for (i = 0; i < NUMBER_POOL_LOOKASIDE_LISTS; i++)
    {
        /* Initialize the non-paged list */
        ExInitializeSystemLookasideList(&ExpSmallNPagedPoolLookasideLists[i],
                                        NonPagedPool,
                                        (i + 1) * 8,
                                        'looP',
                                        256,
                                        &ExPoolLookasideListHead);

        /* Initialize the paged list */
        ExInitializeSystemLookasideList(&ExpSmallPagedPoolLookasideLists[i],
                                        PagedPool,
                                        (i + 1) * 8,
                                        'looP',
                                        256,
                                        &ExPoolLookasideListHead);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
ExiAllocateFromPagedLookasideList(IN PPAGED_LOOKASIDE_LIST Lookaside)
{
    PVOID Entry;

    Lookaside->L.TotalAllocates++;
    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
    if (!Entry)
    {
        Lookaside->L.AllocateMisses++;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }
    return Entry;
}

/*
 * @implemented
 */
VOID
NTAPI
ExiFreeToPagedLookasideList(IN PPAGED_LOOKASIDE_LIST  Lookaside,
                            IN PVOID  Entry)
{
    Lookaside->L.TotalFrees++;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth)
    {
        Lookaside->L.FreeMisses++;
        (Lookaside->L.Free)(Entry);
    }
    else
    {
        InterlockedPushEntrySList(&Lookaside->L.ListHead, (PSLIST_ENTRY)Entry);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
ExDeleteNPagedLookasideList(IN PNPAGED_LOOKASIDE_LIST Lookaside)
{
    KIRQL OldIrql;
    PVOID Entry;

    /* Pop all entries off the stack and release their resources */
    for (;;)
    {
        Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
        if (!Entry) break;
        (*Lookaside->L.Free)(Entry);
    }

    /* Remove from list */
    KeAcquireSpinLock(&ExpNonPagedLookasideListLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    KeReleaseSpinLock(&ExpNonPagedLookasideListLock, OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
ExDeletePagedLookasideList(IN PPAGED_LOOKASIDE_LIST Lookaside)
{
    KIRQL OldIrql;
    PVOID Entry;

    /* Pop all entries off the stack and release their resources */
    for (;;)
    {
        Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
        if (!Entry) break;
        (*Lookaside->L.Free)(Entry);
    }

    /* Remove from list */
    KeAcquireSpinLock(&ExpPagedLookasideListLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    KeReleaseSpinLock(&ExpPagedLookasideListLock, OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
ExInitializeNPagedLookasideList(IN PNPAGED_LOOKASIDE_LIST Lookaside,
                                IN PALLOCATE_FUNCTION Allocate OPTIONAL,
                                IN PFREE_FUNCTION Free OPTIONAL,
                                IN ULONG Flags,
                                IN SIZE_T Size,
                                IN ULONG Tag,
                                IN USHORT Depth)
{
    /* Initialize the Header */
    ExInitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = NonPagedPool | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = (ULONG)Size;
    Lookaside->L.Depth = 4;
    Lookaside->L.MaximumDepth = 256;
    Lookaside->L.LastTotalAllocates = 0;
    Lookaside->L.LastAllocateMisses = 0;

    /* Set the Allocate/Free Routines */
    if (Allocate)
    {
        Lookaside->L.Allocate = Allocate;
    }
    else
    {
        Lookaside->L.Allocate = ExAllocatePoolWithTag;
    }

    if (Free)
    {
        Lookaside->L.Free = Free;
    }
    else
    {
        Lookaside->L.Free = ExFreePool;
    }

    /* Insert it into the list */
    ExInterlockedInsertTailList(&ExpNonPagedLookasideListHead,
                                &Lookaside->L.ListEntry,
                                &ExpNonPagedLookasideListLock);
}

/*
 * @implemented
 */
VOID
NTAPI
ExInitializePagedLookasideList(IN PPAGED_LOOKASIDE_LIST Lookaside,
                               IN PALLOCATE_FUNCTION Allocate OPTIONAL,
                               IN PFREE_FUNCTION Free OPTIONAL,
                               IN ULONG Flags,
                               IN SIZE_T Size,
                               IN ULONG Tag,
                               IN USHORT Depth)
{
    /* Initialize the Header */
    ExInitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = PagedPool | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = (ULONG)Size;
    Lookaside->L.Depth = 4;
    Lookaside->L.MaximumDepth = 256;
    Lookaside->L.LastTotalAllocates = 0;
    Lookaside->L.LastAllocateMisses = 0;

    /* Set the Allocate/Free Routines */
    if (Allocate)
    {
        Lookaside->L.Allocate = Allocate;
    }
    else
    {
        Lookaside->L.Allocate = ExAllocatePoolWithTag;
    }

    if (Free)
    {
        Lookaside->L.Free = Free;
    }
    else
    {
        Lookaside->L.Free = ExFreePool;
    }

    /* Insert it into the list */
    ExInterlockedInsertTailList(&ExpPagedLookasideListHead,
                                &Lookaside->L.ListEntry,
                                &ExpPagedLookasideListLock);
}

PVOID
NTAPI
ExAllocatePoolLookasideEx(_In_    POOL_TYPE PoolType,
                          _In_    SIZE_T NumberOfBytes,
                          _In_    ULONG Tag,
                          _Inout_ PLOOKASIDE_LIST_EX Lookaside)
{
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

VOID
NTAPI
ExFreePoolLookasideEx(PVOID Buffer, PLOOKASIDE_LIST_EX Lookaside)
{
    ExFreePoolWithTag(Buffer, 0);
}

/*
 * @implemented - Vista+
 */
NTSTATUS
NTAPI
ExInitializeLookasideListEx(PLOOKASIDE_LIST_EX Lookaside,
                            PALLOCATE_FUNCTION_EX Allocate,
                            PFREE_FUNCTION_EX Free,
                            POOL_TYPE PoolType,
                            ULONG Flags,
                            SIZE_T Size,
                            ULONG Tag,
                            USHORT Depth)
{
    /* Initialize the Header */
    ExInitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = PoolType | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = (ULONG)Size;
    Lookaside->L.LastTotalAllocates = 0;
    Lookaside->L.LastAllocateMisses = 0;

    //TODO: These values seems a little different on actual vista with SOME drivers (specifically WDDM?)
    Lookaside->L.Depth = 4;
    Lookaside->L.MaximumDepth = 256;

    /* Set the Allocate/Free Routines */
    if (Allocate)
    {
        Lookaside->L.AllocateEx = Allocate;
    }
    else
    {
        Lookaside->L.AllocateEx = ExAllocatePoolLookasideEx;
    }

    if (Free)
    {
        Lookaside->L.FreeEx = Free;
    }
    else
    {
        Lookaside->L.FreeEx = ExFreePoolLookasideEx;
    }

    /* Only except to usual non-paged pools is if we explicitly say we're paged */
    if (PoolType == PagedPool)
    {
        /* Insert it into the list */
        ExInterlockedInsertTailList(&ExpPagedLookasideListHead,
                                    &Lookaside->L.ListEntry,
                                    &ExpPagedLookasideListLock);
    }
    else
    {
         /* Insert it into the list */
        ExInterlockedInsertTailList(&ExpNonPagedLookasideListHead,
                                    &Lookaside->L.ListEntry,
                                    &ExpNonPagedLookasideListLock);
    }

    //TODO: Let's do some error checking?
    return STATUS_SUCCESS;
}

VOID
NTAPI
ExDeleteLookasideListEx(PLOOKASIDE_LIST_EX Lookaside)
{
    //TODO Hella Leaks bros
    UNIMPLEMENTED_ONCE;
}

/* EOF */

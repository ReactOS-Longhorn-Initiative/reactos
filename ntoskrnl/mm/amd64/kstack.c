/*
 * COPYRIGHT:       GPL, See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/amd64/kstack.c
 * PURPOSE:         Kernel stack allocator for AMD64
 *
 * PROGRAMMERS:     Timo kreuzer (timo.kreuzer@reactos.org)
 */

 /* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <mm/ARM3/miarm.h>
#include <fltkernel.h>

#define MI_STACK_PAGES 32
#define MI_STACK_SIZE (MI_STACK_PAGES * PAGE_SIZE)

PMMPTE MiStackPteBaseByLevel[3];
PMMPTE MiNextFreeStackPteByLevel[3];
KSPIN_LOCK MiStackPteSpinLock;

static
NTSTATUS
MapPageTable(PMMPTE PointerPte)
{
    PFN_NUMBER PageFrameNumber;
    MMPTE TempPte;
    KIRQL OldIrql;

    ASSERT(!PointerPte->u.Hard.Valid);

    OldIrql = MiAcquirePfnLock();

    PageFrameNumber = MiRemoveZeroPage(MI_GET_NEXT_COLOR());
    if (PageFrameNumber == 0)
    {
        return STATUS_NO_MEMORY;
    }
    DbgPrint("## MAP %p PFN 0x%lx\n", PointerPte, PageFrameNumber);
    MiInitializePfn(PageFrameNumber, PointerPte, TRUE);
    PMMPFN Pfn = MiGetPfnEntry(PageFrameNumber);
    Pfn->Dbg1.IsStackPfn = 1;

    MiReleasePfnLock(OldIrql);

    MI_MAKE_HARDWARE_PTE(&TempPte,
                         PointerPte,
                         MM_EXECUTE_READWRITE,
                         PageFrameNumber);

    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    return STATUS_SUCCESS;
}

static
VOID
InsertPtesInList(
    _In_ PMMPTE FirstPte,
    _In_ ULONG NumberOfPtes,
    _In_ ULONG Level)
{
    const ULONG PteDelta = (Level == 0) ? MI_STACK_PAGES : 1;
    const PMMPTE LastPte = FirstPte + NumberOfPtes - PteDelta;
    PMMPTE PointerPte;
    ULONG NextEntryOffset;

    ASSERT(Level < 3);
    ASSERT((Level == 2) || IS_ALIGNED(FirstPte, PAGE_SIZE));

    LastPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;
    NextEntryOffset = LastPte - MiStackPteBaseByLevel[Level];

    /* Link the PTEs to a list */
    for (PointerPte = LastPte - PteDelta;
         PointerPte >= FirstPte;
         PointerPte -= PteDelta)
    {
        ASSERT(PointerPte->u.Long == 0);
        PointerPte->u.List.NextEntry = NextEntryOffset;
        NextEntryOffset -= PteDelta;
    }

    MiNextFreeStackPteByLevel[Level] = FirstPte;
}

VOID
MiInitializeStackAllocator(
   VOID)
{
    PMMPPE PointerPpe;
    ULONG NumberOfPpes;

    PVOID BaseAddress = MiSystemVaRegions[AssignedRegionKernelStacks].BaseAddress;
    SIZE_T SizeInBytes = MiSystemVaRegions[AssignedRegionKernelStacks].NumberOfBytes;
    ASSERT(IS_ALIGNED(BaseAddress, PPE_MAPPED_VA));
    ASSERT(MiAddressToPxe(BaseAddress)->u.Hard.Valid);

    /* Initialize the PTE base per level */
    MiStackPteBaseByLevel[0] = MiAddressToPte(BaseAddress);
    MiStackPteBaseByLevel[1] = MiAddressToPde(BaseAddress);
    MiStackPteBaseByLevel[2] = MiAddressToPpe(BaseAddress);

    /* Link the PPEs */
    PointerPpe = MiAddressToPpe(BaseAddress);
    NumberOfPpes = SizeInBytes / PPE_MAPPED_VA;
    InsertPtesInList(PointerPpe, NumberOfPpes, 2);
}

static
PMMPTE
AllocateStackPtes(ULONG Level)
{
    PMMPTE PointerPte;
    PMMPDE PointerPde;
    ULONG NextPteOffset;
    NTSTATUS Status;

    /* Bail out if we reached PXE level */
    if (Level == 3)
    {
        return NULL;
    }

    PointerPte = MiNextFreeStackPteByLevel[Level];
    if (PointerPte == NULL)
    {
        PointerPde = AllocateStackPtes(Level + 1);
        if (PointerPde == NULL)
        {
            return NULL;
        }

        Status = MapPageTable(PointerPde);
        if (!NT_SUCCESS(Status))
        {
            __debugbreak();
            return NULL;
        }

        PointerPte = MiPdeToPte(PointerPde);
        InsertPtesInList(PointerPte, PTE_PER_PAGE, Level);
    }

    NextPteOffset = PointerPte->u.List.NextEntry;
    if (NextPteOffset != MM_EMPTY_PTE_LIST)
    {
        MiNextFreeStackPteByLevel[Level] =
            MiStackPteBaseByLevel[Level] + NextPteOffset;
    }
    else
    {
        MiNextFreeStackPteByLevel[Level] = NULL;
    }

    return PointerPte;
}

PMMPTE
MiReserveKernelStackPtes(
    _In_ ULONG NumberOfPtes)
{
    PMMPTE PointerPte;
    KIRQL OldIrql;

    ASSERT(NumberOfPtes <= MI_STACK_PAGES);

    KeAcquireSpinLock(&MiStackPteSpinLock, &OldIrql);

    PointerPte = AllocateStackPtes(0);

    KeReleaseSpinLock(&MiStackPteSpinLock, OldIrql);

    return PointerPte;
}

VOID
MiReleaseKernelStackPtes(
    _In_ PMMPTE FirstPte,
    _In_ ULONG NumberOfPtes)
{
    PMMPTE PointerPte;
    KIRQL OldIrql;

    ASSERT(NumberOfPtes <= MI_STACK_PAGES);
    ASSERT(IS_ALIGNED(MiPteToAddress(FirstPte), MI_STACK_SIZE));

    /* Zero the PTEs */
    RtlZeroMemory(FirstPte, MI_STACK_PAGES * sizeof(MMPTE));

    KeAcquireSpinLock(&MiStackPteSpinLock, &OldIrql);

    PointerPte = MiNextFreeStackPteByLevel[0];
    if (PointerPte != NULL)
    {
        FirstPte->u.List.NextEntry = PointerPte - MiStackPteBaseByLevel[0];
    }
    else
    {
        FirstPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;
    }

    MiNextFreeStackPteByLevel[0] = FirstPte;

    KeReleaseSpinLock(&MiStackPteSpinLock, OldIrql);
}

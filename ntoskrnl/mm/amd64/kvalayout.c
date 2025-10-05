/*
 * COPYRIGHT:       GPL, See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/amd64/kvalayout.c
 * PURPOSE:         Kernel virtual address layout for AMD64
 *
 * PROGRAMMERS:     Timo kreuzer (timo.kreuzer@reactos.org)
 */

 /* INCLUDES ***************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <mm/ARM3/miarm.h>
#include <fltkernel.h>

/* Random seed */
ULONG MiRandomSeed = 'MRnd';

// FIXME: Should be part of MI_VISIBLE_STATE
MI_SYSTEM_VA_ASSIGNMENT MiSystemVaRegions[AssignedRegionMaximum];

ULONG MiSystemVaAssignment[8];
static RTL_BITMAP MiSystemVaAssignmentBitmap = { 256, MiSystemVaAssignment };

static
VOID
ReservePxi(
    _In_ ULONG Pxi)
{
    ULONG BitIndex = Pxi - 256;
    ASSERT(BitIndex < 256);
    RtlSetBit(&MiSystemVaAssignmentBitmap, BitIndex);
}

static
VOID
ReserveVaRange(
    _In_ ULONG_PTR StartAddress,
    _In_ ULONG_PTR NumberOfBytes)
{
    ULONG_PTR EndingAddress = StartAddress + NumberOfBytes - 1;
    ULONG Pxi;

    for (Pxi = MiAddressToPxi((PVOID)StartAddress);
         Pxi <= MiAddressToPxi((PVOID)EndingAddress);
         Pxi++)
    {
        ReservePxi(Pxi);
    }
}

static
VOID
ReserveVaRegion(
    _In_ MI_ASSIGNED_REGION_TYPES Region,
    ULONG64 BaseAddress,
    ULONG64 NumberOfBytes)
{
    ReserveVaRange(BaseAddress, NumberOfBytes);
    MiSystemVaRegions[Region].BaseAddress = (PVOID)BaseAddress;
    MiSystemVaRegions[Region].NumberOfBytes = NumberOfBytes;
}

static 
ULONG
AcquireRandomPxiRange(ULONG NumberOfPxis)
{
    ULONG AvailableSlots = 0;
    ULONG SkipCount;
    ULONG Index = 0;

    /* First count available slots */
    for (ULONG i = 0; i < (256 - NumberOfPxis); i++)
    {
        if (RtlAreBitsClear(&MiSystemVaAssignmentBitmap, i, NumberOfPxis))
        {
            AvailableSlots++;
        }
    }

    /* We should have plenty available */
    if (AvailableSlots < 100)
    {
        KeBugCheck(MEMORY_MANAGEMENT);
    }

    /* Get a random skip count */
    SkipCount = RtlRandomEx(&MiRandomSeed);
    SkipCount %= AvailableSlots;

    /* Skip over unavailable and 'SkipCount' available slots */
    while (!RtlAreBitsClear(&MiSystemVaAssignmentBitmap, Index, NumberOfPxis) ||
           (SkipCount-- != 0))
    {
        Index++;
    }

    /* Now set the bits */
    for (ULONG i = 0; i < NumberOfPxis; i++)
    {
        RtlSetBit(&MiSystemVaAssignmentBitmap, Index + i);
    }

    return Index + 256;
}

/*
 * The function:
 * - Allocates only as many PXIs as needed.
 * - Randomizes the location within the PXI range as possible.
 */
static
VOID
RandomizeVaRegion(
    _In_ MI_ASSIGNED_REGION_TYPES Region,
    _In_ ULONG64 NumberOfBytes,
    _In_ ULONG64 Alignment)
{
    ULONG_PTR FullSize = ALIGN_UP_BY(NumberOfBytes, PXE_MAPPED_VA);
    ULONG NumberOfPxis = FullSize / PXE_MAPPED_VA;

    ASSERT(NumberOfPxis != 0);
    ASSERT(Alignment >= PDE_MAPPED_VA);
    ASSERT(Alignment <= PXE_MAPPED_VA);
    ASSERT((Alignment & (Alignment - 1)) == 0);

    /* Get a random PXI range for the region and reserve it */
    ULONG Pxi = AcquireRandomPxiRange(NumberOfPxis);

    /* Calculate the number of available slots inside the PXI range */
    ULONG64 MaxOffset = FullSize - NumberOfBytes;
    ULONG AvailableSlots = 1 + (MaxOffset / Alignment);

    /* Get a random slot index */
    ULONG SlotIndex = RtlRandomEx(&MiRandomSeed) % AvailableSlots;

    /* Calculate the actual base address */
    PVOID BaseAddress = Add2Ptr(MiPxiToAddress(Pxi), SlotIndex * Alignment);

    /* Set up the region */
    MiSystemVaRegions[Region].BaseAddress = (PVOID)BaseAddress;
    MiSystemVaRegions[Region].NumberOfBytes = NumberOfBytes;
}

static
PFN_NUMBER
FindHighestPfnNumber(
    _In_ const LOADER_PARAMETER_BLOCK* LoaderBlock)
{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY ListEntry;
    PFN_NUMBER HighestPfn = 0;

    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        Descriptor = CONTAINING_RECORD(ListEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
        PFN_NUMBER UpperPfn = Descriptor->BasePage + Descriptor->PageCount;
        HighestPfn = max(HighestPfn, UpperPfn);
    }

    return HighestPfn;
}

static
VOID
SetupVaRegions(
    _In_ const LOADER_PARAMETER_BLOCK* LoaderBlock)
{
    SIZE_T BootImageSize;

    /* Reserve the shared user page VA */
    ReserveVaRange(KI_USER_SHARED_DATA, PAGE_SIZE);

    /* Reserve 4 MB for the hal */
    ReserveVaRange(MM_HAL_VA_START, 4 * _1MB);

    /* Reserve 512 GB for the page tables */
    ReserveVaRegion(AssignedRegionPageTables, PTE_BASE, 512 * _1GB);

    /* Reserve 512 GB for hyper space */
    ReserveVaRegion(AssignedRegionHyperSpace, HYPER_SPACE, 512 * _1GB);

    /* Reserve 512 GB for session space */
    ReserveVaRegion(AssignedRegionSession, MI_SESSION_SPACE_START, 512 * _1GB);

    /* Reserve loader mappings */
    BootImageSize = LoaderBlock->Extension->LoaderPagesSpanned * PAGE_SIZE;
    ReserveVaRegion(AssignedRegionSystemImages,
                    MI_LOADER_MAPPINGS,
                    BootImageSize + PAGE_SIZE);

    /* Reserve up to 8 TB for the PFN database */
    PFN_NUMBER HighestPfn = FindHighestPfnNumber(LoaderBlock);
    ULONG64 PfnDbSize = HighestPfn * sizeof(MMPFN) + _1MB;
    RandomizeVaRegion(AssignedRegionPfnDatabase, PfnDbSize, PDE_MAPPED_VA);

    /* Reserve 2 TB system cache */
    RandomizeVaRegion(AssignedRegionSystemCache, 2 * _1TB, 512 * _1GB);

    /* Reserve 128 GB for non-paged pool */
    RandomizeVaRegion(AssignedRegionNonPagedPool, 128 * _1GB, PDE_MAPPED_VA);

    /* Reserve 128 GB for paged pool */
    RandomizeVaRegion(AssignedRegionPagedPool, 128 * _1GB, PDE_MAPPED_VA);

    /* Reserve 128 GB for system PTEs */
    RandomizeVaRegion(AssignedRegionSystemPtes, 128 * _1GB, PDE_MAPPED_VA);

    /* Reserve 128 GB for kernel stacks */
    RandomizeVaRegion(AssignedRegionKernelStacks, 128 * _1GB, PPE_MAPPED_VA);

/*
    AssignedRegionUltraZero = 4,
    AssignedRegionCfg = 6,
    AssignedRegionSecureNonPagedPool = 11,
*/

    MmPagedPoolStart = MiSystemVaRegions[AssignedRegionPagedPool].BaseAddress;
}

static
VOID
RelocatePageTables()
{
    // TODO
}

static
VOID
RelocateBootLoadedImages()
{
    // TODO
}

VOID
NTAPI
MiInitializeKernelVaLayout(
    _In_ const LOADER_PARAMETER_BLOCK* LoaderBlock)
{
    /* Initialize the random seed */
    if (LoaderBlock->Extension->LoaderPerformanceData != 0)
    {
        MiRandomSeed ^= LoaderBlock->Extension->LoaderPerformanceData->StartTime;
        MiRandomSeed ^= _rotl(LoaderBlock->Extension->LoaderPerformanceData->EndTime, 16);
    }
    MiRandomSeed ^= _rotl(__rdtsc(), MiRandomSeed & 0x1F);

    SetupVaRegions(LoaderBlock);

    RelocatePageTables();

    RelocateBootLoadedImages();
}

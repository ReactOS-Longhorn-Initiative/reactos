 
#include <ntdef.h>
#include <ntifs.h>

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes))
NTKRNLVISTAAPI
PVOID
NTAPI
MmAllocateContiguousMemorySpecifyCacheNode(
    _In_ SIZE_T NumberOfBytes,
    _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
    _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _In_ NODE_REQUIREMENT PreferredNode)
{
    return MmAllocateContiguousMemorySpecifyCache(NumberOfBytes,
                                                    LowestAcceptableAddress,
                                                    HighestAcceptableAddress,
                                                    BoundaryAddressMultiple,
                                                    CacheType);
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
_When_ (return != NULL, _Post_writable_byte_size_ (NumberOfBytes))
NTKRNLVISTAAPI
PVOID
NTAPI
MmAllocateContiguousNodeMemory(
    _In_ SIZE_T NumberOfBytes,
    _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
    _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
    _In_ ULONG Protect,
    _In_ NODE_REQUIREMENT PreferredNode)
{
    MEMORY_CACHING_TYPE CacheType;

    switch(Protect)
    {
        case PAGE_NOCACHE:
            CacheType = MmNonCached;
            break;
        case PAGE_WRITECOMBINE:
            CacheType = MmWriteCombined;
            break;
        case NULL:
            CacheType = 0;
            break;
        default:
            return NULL;
    }

    return MmAllocateContiguousMemorySpecifyCache(NumberOfBytes,
                                                    LowestAcceptableAddress,
                                                    HighestAcceptableAddress,
                                                    BoundaryAddressMultiple,
                                                    CacheType);
}

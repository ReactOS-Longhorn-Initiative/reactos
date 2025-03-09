 
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
        case 0:
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


/*********************************************************************
 *                  strncpy_s   (NTDLL.@)
 */
errno_t __cdecl strncpy_s( char *dst, size_t len, const char *src, size_t count )
{
    size_t i, end;

    if (!count)
    {
        if (dst && len) *dst = 0;
        return 0;
    }
    if (!dst || !len) return EINVAL;
    if (!src)
    {
        *dst = 0;
        return EINVAL;
    }

    if (count != _TRUNCATE && count < len)
        end = count;
    else
        end = len - 1;

    for (i = 0; i < end; i++)
        if (!(dst[i] = src[i])) return 0;

    if (count == _TRUNCATE)
    {
        dst[i] = 0;
        return STRUNCATE;
    }
    if (end == count)
    {
        dst[i] = 0;
        return 0;
    }
    dst[0] = 0;
    return ERANGE;
}

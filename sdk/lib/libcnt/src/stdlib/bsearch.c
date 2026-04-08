/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of bsearch
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <search.h>

typedef int (__cdecl* _CoreCrtNonSecureSearchSortCompareFunction)(const void*, const void*);

_Check_return_
void*
__cdecl
bsearch(
    _In_ const void* const Key,
    _In_reads_bytes_(NumOfElements * _SizeOfElements) const void* const Base,
    _In_ size_t const NumOfElements,
    _In_ size_t const SizeOfElements,
    _In_ _CoreCrtNonSecureSearchSortCompareFunction const CompareFunction)
{
    const char* base = (const char*)Base;
    size_t lo = 0;
    size_t hi = NumOfElements;

    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        const void* element = base + (mid * SizeOfElements);

        int result = CompareFunction(Key, element);
        if (result < 0)
        {
            hi = mid;
        }
        else if (result > 0)
        {
            lo = mid + 1;
        }
        else
        {
            return (void*)element;
        }
    }

    return NULL;
}

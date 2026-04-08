/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of bsearch_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <search.h>

typedef int (__cdecl* _CoreCrtSecureSearchSortCompareFunction)(void*, const void*, const void*);

_Check_return_
void*
__cdecl
bsearch_s(
    _In_ const void* const _Key,
    _In_reads_bytes_(_NumOfElements * _SizeOfElements) const void* const _Base,
    _In_ rsize_t const _NumOfElements,
    _In_ rsize_t const _SizeOfElements,
    _In_ _CoreCrtSecureSearchSortCompareFunction const _CompareFunction,
    _In_opt_ void* const _Context)
{
    const char* base = (const char*)_Base;
    rsize_t lo = 0;
    rsize_t hi = _NumOfElements;

    while (lo < hi)
    {
        rsize_t mid = lo + (hi - lo) / 2;
        const void* element = base + (mid * _SizeOfElements);

        int result = _CompareFunction(_Context, _Key, base + mid * _SizeOfElements);
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

/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of qsort
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "qsort.hpp"

typedef int (__cdecl* _CoreCrtNonSecureSearchSortCompareFunction)(const void*, const void*);

extern "C"
void
__cdecl
qsort(
    _Inout_updates_bytes_(NumOfElements * SizeOfElements) void* const Base,
    _In_ size_t const NumOfElements,
    _In_ size_t const SizeOfElements,
    _In_ _CoreCrtNonSecureSearchSortCompareFunction const CompareFunction)
{
    _qsort_impl(
        Base,
        NumOfElements,
        SizeOfElements,
        [CompareFunction](const void* a, const void* b)
        {
            return CompareFunction(a, b);
        });
}

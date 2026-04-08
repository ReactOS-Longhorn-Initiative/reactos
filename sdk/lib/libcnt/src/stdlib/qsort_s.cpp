/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of qsort_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "qsort.hpp"

typedef int (__cdecl* _CoreCrtSecureSearchSortCompareFunction)(void*, const void*, const void*);

extern "C"
void
__cdecl
qsort_s(
    _Inout_updates_bytes_(NumOfElements * SizeOfElements) void* const Base,
    _In_ rsize_t const NumOfElements,
    _In_ rsize_t const SizeOfElements,
    _In_ _CoreCrtSecureSearchSortCompareFunction const CompareFunction,
    _In_opt_ void* const Context)
{
    _qsort_impl(
        Base,
        NumOfElements,
        SizeOfElements,
        [CompareFunction, Context](const void* a, const void* b)
        {
            return CompareFunction(Context, a, b);
        });
}

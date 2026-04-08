/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _lfind
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <search.h>

typedef int (__cdecl* _CoreCrtNonSecureSearchSortCompareFunction)(const void*, const void*);

_Check_return_
void*
__cdecl
_lfind(
    _In_ const void* _Key,
    _In_reads_bytes_((*_NumOfElements) * _SizeOfElements) const void* _Base,
    _Inout_ unsigned int* const _NumOfElements,
    _In_ unsigned int const _SizeOfElements,
    _In_ _CoreCrtNonSecureSearchSortCompareFunction _CompareFunction)
{
    const char* const first = (const char*)_Base;
    const char* const last = first + *_NumOfElements * _SizeOfElements;

    for (const char* p = first; p != last; p += _SizeOfElements)
    {
        if (_CompareFunction(_Key, p) == 0)
        {
            return (void*)p;
        }
    }

    return NULL;
}

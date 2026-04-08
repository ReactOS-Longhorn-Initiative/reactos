/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _memccpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

void*
__cdecl
_memccpy(
    _Out_writes_bytes_opt_(_MaxCount) void* const _Dst,
    _In_ const void* const _Src,
    _In_ int const _Val,
    _In_ size_t const _MaxCount)
{
    unsigned char* dst = (unsigned char*)_Dst;
    const unsigned char* src = (const unsigned char*)_Src;
    unsigned char val = (unsigned char)_Val;
    size_t remaining = _MaxCount;

    while (remaining--)
    {
        *dst = *src;
        dst++;
        if (*src == val)
            return dst;
        src++;
    }

    return NULL;
}

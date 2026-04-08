/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _memicmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
_memicmp(
    _In_reads_bytes_opt_(_Size) const void* const _Buf1,
    _In_reads_bytes_opt_(_Size) const void* const _Buf2,
    _In_ size_t const _Size)
{
    const unsigned char* p1 = (const unsigned char*)_Buf1;
    const unsigned char* p2 = (const unsigned char*)_Buf2;
    unsigned char c1, c2;
    size_t remaining = _Size;

    while (remaining--)
    {
        c1 = *p1++;
        c2 = *p2++;
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 += 'a' - 'A';
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 += 'a' - 'A';
        if (c1 != c2)
            return c1 - c2;
    }

    return 0;
}

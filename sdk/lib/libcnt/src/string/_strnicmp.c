/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strnicmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
_strnicmp(
    _In_reads_or_z_(_MaxCount) const char* const _String1,
    _In_reads_or_z_(_MaxCount) const char* const _String2,
    _In_ size_t const _MaxCount)
{
    const char* str1 = _String1;
    const char* str2 = _String2;
    size_t remaining = _MaxCount;
    unsigned char chr1, chr2;

    while (remaining > 0)
    {
        chr1 = *str1++;
        chr2 = *str2++;
        if ((chr1 >= 'A') && (chr1 <= 'Z'))
            chr1 += 'a' - 'A';
        if ((chr2 >= 'A') && (chr2 <= 'Z'))
            chr2 += 'a' - 'A';
        if (chr1 != chr2)
            return chr1 - chr2;
        if (chr1 == '\0')
            return 0;
        remaining--;
    }

    return 0;
}

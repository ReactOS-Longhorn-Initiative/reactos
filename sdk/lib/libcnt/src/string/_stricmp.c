/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _stricmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
_stricmp(
    _In_z_ const char* const _String1,
    _In_z_ const char* const _String2)
{
    const char* str1 = _String1;
    const char* str2 = _String2;
    unsigned char chr1, chr2;

    do
    {
        chr1 = *str1++;
        chr2 = *str2++;
        if ((chr1 >= 'A') && (chr1 <= 'Z'))
            chr1 += 'a' - 'A';
        if ((chr2 >= 'A') && (chr2 <= 'Z'))
            chr2 += 'a' - 'A';
    } while (chr1 && (chr1 == chr2));

    return chr1 - chr2;
}

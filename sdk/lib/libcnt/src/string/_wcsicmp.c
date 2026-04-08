/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wcsicmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
_wcsicmp(
    _In_z_ const wchar_t* const _String1,
    _In_z_ const wchar_t* const _String2)
{
    const wchar_t* str1 = _String1;
    const wchar_t* str2 = _String2;
    wchar_t chr1, chr2;

    do
    {
        chr1 = *str1++;
        chr2 = *str2++;
        if ((chr1 >= L'A') && (chr1 <= L'Z'))
            chr1 += (L'a' - L'A');
        if ((chr2 >= L'A') && (chr2 <= L'Z'))
            chr2 += (L'a' - L'A');
    } while (chr1 && (chr1 == chr2));

    return chr1 - chr2;
}

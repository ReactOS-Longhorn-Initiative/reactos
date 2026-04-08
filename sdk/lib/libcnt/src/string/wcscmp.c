/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcscmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
wcscmp(
    _In_z_ const wchar_t* const _String1,
    _In_z_ const wchar_t* const _String2)
{
    const wchar_t* str1 = _String1;
    const wchar_t* str2 = _String2;

    while (*str1 == *str2)
    {
        if (*str1 == L'\0')
            return 0;
        str1++;
        str2++;
    }

    return ((unsigned short)*str1 > (unsigned short)*str2) ? 1 : -1;
}

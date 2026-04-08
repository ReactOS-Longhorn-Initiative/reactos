/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcsncmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
wcsncmp(
    _In_reads_or_z_(_MaxCount) const wchar_t* const _String1,
    _In_reads_or_z_(_MaxCount) const wchar_t* const _String2,
    _In_ size_t const _MaxCount)
{
    const wchar_t* str1 = _String1;
    const wchar_t* str2 = _String2;
    size_t remaining = _MaxCount;

    while (remaining > 0)
    {
        if (*str1 != *str2)
            return (unsigned short)*str1 - (unsigned short)*str2;
        if (*str1 == L'\0')
            return 0;
        str1++;
        str2++;
        remaining--;
    }

    return 0;
}

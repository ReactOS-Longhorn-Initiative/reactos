/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wcsnicmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
int
__cdecl
_wcsnicmp(
    _In_reads_or_z_(_MaxCount) const wchar_t* const _String1,
    _In_reads_or_z_(_MaxCount) const wchar_t* const _String2,
    _In_ size_t const _MaxCount)
{
    const wchar_t* str1 = _String1;
    const wchar_t* str2 = _String2;
    size_t remaining = _MaxCount;
    wchar_t chr1, chr2;

    while (remaining > 0)
    {
        chr1 = *str1++;
        chr2 = *str2++;
        if ((chr1 >= L'A') && (chr1 <= L'Z'))
            chr1 += L'a' - L'A';
        if ((chr2 >= L'A') && (chr2 <= L'Z'))
            chr2 += L'a' - L'A';
        if (chr1 != chr2)
            return chr1 - chr2;
        if (chr1 == L'\0')
            return 0;
        remaining--;
    }

    return 0;
}

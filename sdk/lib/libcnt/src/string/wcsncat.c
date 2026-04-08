/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcsncat
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

wchar_t*
__cdecl
wcsncat(
    _Inout_z_ wchar_t* const _Destination,
    _In_reads_or_z_(_Count) const wchar_t* const _Source,
    _In_ size_t const _Count)
{
    wchar_t* dst = _Destination;
    const wchar_t* src = _Source;
    size_t remaining = _Count;

    while (*dst != L'\0')
        dst++;

    while ((remaining > 0) && (*src != L'\0'))
    {
        *dst++ = *src++;
        remaining--;
    }

    *dst = L'\0';

    return _Destination;
}

/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcsncpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

wchar_t*
__cdecl
wcsncpy(
    _Out_writes_(_Count) wchar_t* const _Destination,
    _In_reads_or_z_(_Count) const wchar_t* const _Source,
    _In_ size_t const _Count)
{
    wchar_t* dst = _Destination;
    const wchar_t* src = _Source;
    size_t remaining = _Count;

    while ((remaining) && ((*dst = *src) != L'\0'))
    {
        dst++;
        src++;
        remaining--;
    }

    /* Pad remaining with nulls */
    while (remaining--)
        *dst++ = L'\0';

    return _Destination;
}

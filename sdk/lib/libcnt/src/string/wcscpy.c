/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcscpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

wchar_t*
__cdecl
wcscpy(
    _Out_writes_z_(_String_length_(_Source) + 1) wchar_t* const _Destination,
    _In_z_ const wchar_t* const _Source)
{
    wchar_t* dst = _Destination;
    const wchar_t* src = _Source;

    while ((*dst++ = *src++) != L'\0')
        ;

    return _Destination;
}

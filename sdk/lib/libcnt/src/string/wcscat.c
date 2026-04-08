/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcscat
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

wchar_t*
__cdecl
wcscat(
    _Inout_z_ wchar_t* const _Destination,
    _In_z_ const wchar_t* const _Source)
{
    wchar_t* dst = _Destination;
    const wchar_t* src = _Source;

    while (*dst != L'\0')
        dst++;

    while ((*dst++ = *src++) != L'\0')
        ;

    return _Destination;
}

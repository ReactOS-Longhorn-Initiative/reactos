/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wcsrev
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

wchar_t*
__cdecl
_wcsrev(
    _Inout_z_ wchar_t* _Str)
{
    wchar_t* start = _Str;
    wchar_t* end = _Str + wcslen(_Str) - 1;

    while (start < end)
    {
        wchar_t temp = *start;
        *start++ = *end;
        *end-- = temp;
    }

    return _Str;
}

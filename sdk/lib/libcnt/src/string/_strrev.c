/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strrev
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

char*
__cdecl
_strrev(
    _Inout_z_ char* const _Str)
{
    char* start = _Str;
    char* end = _Str + strlen(_Str) - 1;

    while (start < end)
    {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }

    return _Str;
}

/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _vscprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include "_output.hpp"

extern "C"
_Check_return_
int
__cdecl
_vscprintf(
    _In_z_ _Printf_format_string_ char const* const Format,
    va_list const ArgList
    )
{
    int const result = _tcsoutput<char>(NULL, 0, Format, ArgList);
    return result < 0 ? -1 : result;
}

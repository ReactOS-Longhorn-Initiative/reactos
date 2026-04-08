/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _vscwprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include "_output.hpp"

extern "C"
_Success_(return >= 0)
_Check_return_
int
__cdecl
_vscwprintf(
    _In_z_ _Printf_format_string_ const wchar_t* const Format,
    va_list const ArgList
    )
{
    int const result = _tcsoutput<wchar_t>(NULL, 0, Format, ArgList);
    return result < 0 ? -1 : result;
}

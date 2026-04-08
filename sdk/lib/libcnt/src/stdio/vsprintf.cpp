/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _vsnwprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include "_output.hpp"

extern "C"
_Success_(return >= 0)
int
__cdecl
vsprintf(
    _Pre_notnull_ _Always_(_Post_z_) char* const Buffer,
    _In_z_ _Printf_format_string_ const char* const Format,
    va_list const ArgList)
{
    int const result = _tcsoutput<char>(Buffer, (size_t)-1, Format, ArgList);
    return result < 0 ? -1 : result;
}

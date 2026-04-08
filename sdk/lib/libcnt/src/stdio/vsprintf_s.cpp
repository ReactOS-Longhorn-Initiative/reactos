/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of vsprintf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include "_output.hpp"

extern "C"
_Success_(return >= 0)
_Check_return_opt_
int
__cdecl
vsprintf_s(
    _Out_writes_(_BufferCount) _Always_(_Post_z_) char* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Printf_format_string_ const char* const Format,
    va_list const ArgList)
{
    return _tcsoutput_s<char>(Buffer, BufferCount, (size_t)-2, Format, ArgList);
}

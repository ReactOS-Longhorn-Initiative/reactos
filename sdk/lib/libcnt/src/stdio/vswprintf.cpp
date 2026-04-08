/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of vswprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include "_output.hpp"

extern "C"
_Success_(return >= 0)
_Check_return_opt_
int
__cdecl
vswprintf(
    _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Printf_format_string_params_(1) const wchar_t* const Format,
    va_list const ArgList)
{
    int const result = _tcsoutput<wchar_t>(Buffer, BufferCount, Format, ArgList);
    return result < 0 ? -1 : result;
}

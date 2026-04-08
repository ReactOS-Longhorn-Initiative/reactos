/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _snwprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include <stdarg.h>

extern "C"
_Success_(return >= 0)
_Check_return_opt_
int
__cdecl
_snwprintf(
    _Out_writes_opt_(_BufferCount) _Post_maybez_ wchar_t* const Buffer,
    _In_ size_t const BufferCount,
    _In_z_ _Printf_format_string_ const wchar_t* const Format,
    ...)
{
    int result;
    va_list args;
    va_start(args, Format);
    result = _vsnwprintf(Buffer, BufferCount, Format, args);
    va_end(args);
    return result;
}

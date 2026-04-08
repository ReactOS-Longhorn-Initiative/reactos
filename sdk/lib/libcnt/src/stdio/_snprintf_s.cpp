/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _snprintf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include <stdarg.h>

extern "C"
_Success_(return >= 0)
_Check_return_opt_
int
__cdecl
_snprintf_s(
    _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) char* const Buffer,
    _In_ size_t const BufferCount,
    _In_ size_t const MaxCount,
    _In_z_ _Printf_format_string_ const char* const Format,
    ...)
{
    int result;
    va_list args;
    va_start(args, Format);
    result = _vsnprintf_s(Buffer, BufferCount, MaxCount, Format, args);
    va_end(args);
    return result;
}

/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of swprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include <stdarg.h>

extern "C"
_Success_(return >= 0)
_Check_return_opt_
int
__cdecl
swprintf(
    _Out_writes_opt_(_BufferCount) _Always_(_Post_z_) wchar_t* const _Buffer,
    _In_ size_t const _BufferCount,
    _In_z_ _Printf_format_string_ const wchar_t* const _Format,
    ...)
{
    int result;
    va_list args;
    va_start(args, _Format);
    result = _vsnwprintf(_Buffer, _BufferCount, _Format, args);
    va_end(args);
    return result;
}

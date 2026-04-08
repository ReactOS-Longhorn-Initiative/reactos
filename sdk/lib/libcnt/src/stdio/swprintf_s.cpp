/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of swprintf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include <stdarg.h>

extern "C"
_Success_(return >= 0)
int
__cdecl
swprintf_s(
    _Out_writes_(_BufferCount) _Always_(_Post_z_) wchar_t* const _Buffer,
    _In_ size_t const _BufferCount,
    _In_z_ _Printf_format_string_ const wchar_t* const _Format,
    ...)
{
    int result;
    va_list args;
    __crt_va_start(args, _Format);
    result = vswprintf_s(_Buffer, _BufferCount, _Format, args);
    __crt_va_end(args);
    return result;
}

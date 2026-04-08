/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _snscanf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_input.hpp"
#include <stdio.h>
#include <stdarg.h>

extern "C"
_Check_return_opt_
int
__cdecl
_snscanf(
    _In_reads_bytes_(_BufferCount) _Pre_z_ char const* const _Buffer,
    _In_ size_t const _BufferCount,
    _In_z_ _Scanf_format_string_ char const* const _Format,
    ...)
{
    int result;
    va_list args;

    va_start(args, _Format);
    result = _tcsinput<char>(_Buffer, _BufferCount, _Format, args);
    va_end(args);

    return result;
}

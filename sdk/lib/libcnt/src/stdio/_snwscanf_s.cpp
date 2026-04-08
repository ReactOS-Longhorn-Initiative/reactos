/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _snwscanf_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_input.hpp"
#include <stdio.h>
#include <stdarg.h>

extern "C"
_Success_(return >= 0)
_Check_return_opt_
int
__cdecl
_snwscanf_s(
    _In_reads_(_BufferCount) _Pre_z_ const wchar_t* const _Buffer,
    _In_ size_t const _BufferCount,
    _In_z_ _Scanf_s_format_string_ const wchar_t* const _Format,
    ...)
{
    int result;
    va_list args;

    va_start(args, _Format);
    result = _tcsinput_s<wchar_t>(_Buffer, _BufferCount, _Format, args);
    va_end(args);

    return result;
}

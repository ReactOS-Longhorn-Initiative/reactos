/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of sscanf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_input.hpp"
#include <stdio.h>
#include <stdarg.h>

extern "C"
_Success_(return >= 0)
_Check_return_
int
__cdecl
sscanf(
    _In_z_ const char* const _Buffer,
    _In_z_ _Scanf_format_string_ const char* const _Format,
    ...)
{
    int result;
    va_list args;

    va_start(args, _Format);
    result = _tcsinput<char>(_Buffer, (size_t)-1, _Format, args);
    va_end(args);

    return result;
}

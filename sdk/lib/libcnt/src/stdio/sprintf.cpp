/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of sprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdio.h>
#include <stdarg.h>

extern "C"
_Success_(return >= 0)
_Check_return_opt_
int
__cdecl
sprintf(
    _Pre_notnull_ _Always_(_Post_z_) char* const _Buffer,
    _In_z_ _Printf_format_string_ const char* const _Format,
    ...)
{
    int result;
    va_list args;
    va_start(args, _Format);
    result = vsprintf(_Buffer, _Format, args);
    va_end(args);
    return result;
}

/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _invalid_parameter
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <intrin.h>

// Avoid including internal headers
unsigned long
__cdecl
DbgPrint(
    _In_z_ _Printf_format_string_ const char* Format,
    ...);

void
__cdecl
_invalid_parameter(
    _In_opt_z_ wchar_t const* const _Expression,
    _In_opt_z_ wchar_t const* const _FunctionName,
    _In_opt_z_ wchar_t const* const _FileName,
    _In_ unsigned int const _LineNumber,
    _In_ uintptr_t const _Reserved)
{
    DbgPrint("%ws:%u: Invalid parameter ('%ws') was passed to C runtime function %ws.\n",
             _FileName,
             _LineNumber,
             _Expression,
             _FunctionName);
}

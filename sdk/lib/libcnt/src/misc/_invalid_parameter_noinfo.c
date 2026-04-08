/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _invalid_parameter_noinfo
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <intrin.h>

// Avoid including internal headers
unsigned long
__cdecl
DbgPrint(
    _In_z_ _Printf_format_string_ const char* Format,
    ...);

void __cdecl _invalid_parameter_noinfo(void)
{
    DbgPrint("Invalid parameter was passed to C runtime function at address %p.\n",
             _ReturnAddress());
}

/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of towupper
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#include "../ntrtl.h"

_Check_return_
wint_t
__cdecl
towupper(
    _In_ wint_t _C)
{
    return RtlUpcaseUnicodeChar((WCHAR)_C);
}

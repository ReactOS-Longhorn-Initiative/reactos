/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of iswascii
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#undef iswascii

_Check_return_ int __cdecl iswascii(_In_ wint_t _C)
{
    return (_C <= 0x7F);
}

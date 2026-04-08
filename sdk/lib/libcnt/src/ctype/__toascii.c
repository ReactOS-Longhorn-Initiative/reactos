/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __toascii
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#undef __toascii

_Check_return_ int __cdecl __toascii(_In_ int _C)
{
    return (_C & 0x7f);
}

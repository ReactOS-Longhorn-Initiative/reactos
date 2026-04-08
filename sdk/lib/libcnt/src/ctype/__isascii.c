/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __isascii function
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#undef __isascii

_Check_return_ int __cdecl __isascii(_In_ int c)
{
    return ((unsigned int)c <= 0x7f);
}

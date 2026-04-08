/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __iswcsymf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#undef __iswcsymf

_Check_return_ int __cdecl __iswcsymf(_In_ wint_t C)
{
    return (iswalpha(C) || (C == '_'));
}

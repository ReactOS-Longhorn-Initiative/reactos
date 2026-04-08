/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __iswcsym
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#undef __iswcsym

_Check_return_ int __cdecl __iswcsym(_In_ wint_t C)
{
    return iswalnum(C) || (C == '_');
}

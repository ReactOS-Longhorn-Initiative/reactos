/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __iscsym
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#undef __iscsym

_Check_return_ int __cdecl __iscsym(_In_ int _C)
{
    char c = (char)_C;
    return isalnum(c) || (c == '_');
}

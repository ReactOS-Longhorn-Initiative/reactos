/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of __iscsymf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#undef __iscsymf

_Check_return_ int __cdecl __iscsymf(_In_ int _C)
{
    return (isalpha(_C) || ((_C) == '_'));
}

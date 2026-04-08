/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of abs
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>

_Check_return_
int
__cdecl
abs(_In_ int const _Number)
{
    return (_Number >= 0) ? _Number : -_Number;
}

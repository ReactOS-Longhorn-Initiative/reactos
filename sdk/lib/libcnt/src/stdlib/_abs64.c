/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _abs64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>

_Check_return_
__int64
__cdecl
_abs64(_In_ __int64 const _Number)
{
    return (_Number >= 0) ? _Number : -_Number;
}

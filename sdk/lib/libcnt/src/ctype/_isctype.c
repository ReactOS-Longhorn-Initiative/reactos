/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _isctype
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_ctype.h"

_Check_return_ int __cdecl _isctype(_In_ int _C, _In_ int _Type)
{
    return __inline_isctype(_C, _Type);
}

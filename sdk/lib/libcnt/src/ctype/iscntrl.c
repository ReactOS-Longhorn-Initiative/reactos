/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of iscntrl
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_ctype.h"
#undef iscntrl

_Check_return_ int __cdecl iscntrl(_In_ int _C)
{
    return __inline_isctype(_C, _CONTROL);
}

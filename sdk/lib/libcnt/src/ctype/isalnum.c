/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of isalnum
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_ctype.h"
#undef isalnum

_Check_return_ int __cdecl isalnum(_In_ int _C)
{
    return __inline_isctype(_C, _ALPHA | _DIGIT);
}

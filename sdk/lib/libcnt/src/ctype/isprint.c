/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of isprint
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_ctype.h"
#undef isprint

_Check_return_ int __cdecl isprint(_In_ int _C)
{
    return __inline_isctype(_C, _BLANK | _PUNCT | _ALPHA | _DIGIT);
}

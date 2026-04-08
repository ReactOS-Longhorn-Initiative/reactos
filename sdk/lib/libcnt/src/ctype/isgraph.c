/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of isgraph
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_ctype.h"
#undef isgraph

_Check_return_ int __cdecl isgraph(_In_ int _C)
{
    return __inline_isctype(_C, _PUNCT | _ALPHA | _DIGIT);
}

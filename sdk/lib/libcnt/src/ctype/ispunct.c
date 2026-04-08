/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of ispunct
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_ctype.h"
#undef ispunct

_Check_return_ int __cdecl ispunct(_In_ int _C)
{
    return __inline_isctype(_C, _PUNCT);
}

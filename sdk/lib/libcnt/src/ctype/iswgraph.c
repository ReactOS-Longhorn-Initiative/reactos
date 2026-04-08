/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of iswgraph
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_wctype.h"
#undef iswgraph

_Check_return_ int __cdecl iswgraph(_In_ wint_t _C)
{
    return __inline_iswctype(_C, _PUNCT | _ALPHA | _DIGIT);
}

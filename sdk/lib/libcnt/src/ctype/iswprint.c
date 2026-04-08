/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of iswprint
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_wctype.h"
#undef iswprint

_Check_return_ int __cdecl iswprint(_In_ wint_t _C)
{
    return __inline_iswctype(_C, _BLANK | _PUNCT | _ALPHA | _DIGIT);
}

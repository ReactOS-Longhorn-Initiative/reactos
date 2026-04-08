/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of iswpunct
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_wctype.h"
#undef iswpunct

_Check_return_ int __cdecl iswpunct(_In_ wint_t _C)
{
    return __inline_iswctype(_C, _PUNCT);
}

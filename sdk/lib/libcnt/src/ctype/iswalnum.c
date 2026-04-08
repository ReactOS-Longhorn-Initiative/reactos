/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of iswalnum
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_wctype.h"
#undef iswalnum

_Check_return_ int __cdecl iswalnum(_In_ wint_t _C)
{
    return __inline_iswctype(_C, _ALPHA | _DIGIT);
}

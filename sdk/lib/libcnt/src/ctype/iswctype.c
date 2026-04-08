/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of iswctype
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "_wctype.h"

_Check_return_ int __cdecl iswctype(_In_ wint_t _C, _In_ wctype_t _Type)
{
    return __inline_iswctype(_C, _Type);
}

/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of towlower
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#include "_wctype.h"

_Check_return_
wint_t
__cdecl
towlower(
    _In_ wint_t _C)
{
    if (__inline_iswctype(_C, _UPPER))
    {
        return _C + ('a' - 'A');
    }

    return _C;
}

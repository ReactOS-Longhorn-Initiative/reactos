/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Internal header for _wctype data
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */


#include <ctype.h>

extern const unsigned short _wctype[257];

static inline int __cdecl __inline_iswctype(wint_t _C, int _Type)
{
    if (_C > 0xFF) return 0;
    return _wctype[_C + 1] & _Type;
}

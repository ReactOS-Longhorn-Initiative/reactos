/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Internal header for _ctype data
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */


#include <ctype.h>

extern const unsigned short _ctype[257];

static inline int __cdecl __inline_isctype(int _C, int _Type)
{
    return _ctype[(_C & 0xFF) + 1] & _Type;
}

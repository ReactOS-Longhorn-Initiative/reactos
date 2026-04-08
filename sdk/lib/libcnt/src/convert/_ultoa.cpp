/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _ultoa
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "xtostr_impl.hpp"

extern "C"
char*
__cdecl
_ultoa(
    _In_ unsigned long const _Value,
    _Pre_notnull_ _Post_z_ char* const _Buffer,
    _In_ int const _Radix)
{
    return _xtotcs<uint32_t, char>(_Value, _Buffer, _Radix);
}

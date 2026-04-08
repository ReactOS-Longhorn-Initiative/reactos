/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _itow
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "xtostr_impl.hpp"

extern "C"
wchar_t*
__cdecl
_itow(
    _In_ int const _Value,
    _Pre_notnull_ _Post_z_ wchar_t* const _Buffer,
    _In_ int const _Radix)
{
    return _xtotcs<int32_t, wchar_t>(_Value, _Buffer, _Radix);
}

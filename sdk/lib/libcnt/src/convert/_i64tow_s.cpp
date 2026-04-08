/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _i64tow_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "xtostr_impl.hpp"

extern "C"
_Check_return_wat_
errno_t
__cdecl
_i64tow_s(
    _In_ __int64 const _Value,
    _Out_writes_z_(_BufferCount) wchar_t* const _Buffer,
    _In_ size_t const _BufferCount,
    _In_ int const _Radix)
{
    return _xtotcs_s<int64_t, wchar_t>(_Value, _Buffer, _BufferCount, _Radix);
}

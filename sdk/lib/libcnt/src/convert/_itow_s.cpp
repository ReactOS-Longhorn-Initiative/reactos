/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _itow_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "xtostr_impl.hpp"

extern "C"
_Success_(return == 0)
_Check_return_wat_
errno_t
__cdecl
_itow_s(
    _In_ int const _Value,
    _Out_writes_z_(_BufferCount) wchar_t* const _Buffer,
    _In_ size_t const _BufferCount,
    _In_ int const _Radix)
{
    return _xtotcs_s<int32_t, wchar_t>(_Value, _Buffer, _BufferCount, _Radix);
}

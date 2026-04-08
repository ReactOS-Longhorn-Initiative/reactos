/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _i64toa_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "xtostr_impl.hpp"

extern "C"
_Success_(return == 0)
_Check_return_opt_
errno_t
__cdecl
_i64toa_s(
    _In_ __int64 const _Value,
    _Out_writes_z_(_BufferCount) char* const _Buffer,
    _In_ size_t const _BufferCount,
    _In_ int const _Radix)
{
    return _xtotcs_s<int64_t, char>(_Value, _Buffer, _BufferCount, _Radix);
}

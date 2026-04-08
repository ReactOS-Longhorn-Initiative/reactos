/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wcstoui64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "tcstox_impl.hpp"

extern "C"
_Check_return_
unsigned __int64
__cdecl
_wcstoui64(
    _In_z_ const wchar_t* const _String,
    _Out_opt_ _Deref_post_z_ wchar_t** const _EndPtr,
    _In_ int const _Radix)
{
    return _tcstox_impl<unsigned __int64, wchar_t>(_String, _EndPtr, _Radix, false);
}

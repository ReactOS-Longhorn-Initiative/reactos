/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wtoi64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "tcstox_impl.hpp"

extern "C"
_Check_return_
__int64
__cdecl
_wtoi64(
    _In_z_ const wchar_t* const _String)
{
    return _tcstox_impl<__int64, wchar_t>(_String, NULL, 10, true);
}

/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wtoi
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "tcstox_impl.hpp"

extern "C"
_Check_return_
int
__cdecl
_wtoi(
    _In_z_ const wchar_t* const _String)
{
    return _atox_nosat_impl<int, wchar_t>(_String);
}

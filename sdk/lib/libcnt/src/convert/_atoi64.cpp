/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _atoi64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "tcstox_impl.hpp"

extern "C"
_Check_return_
__int64
__cdecl
_atoi64(
    _In_z_ const char* const _String)
{
    return _tcstox_impl<__int64, char>(_String, NULL, 10, true);
}

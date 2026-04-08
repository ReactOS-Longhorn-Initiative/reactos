/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of atol
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "tcstox_impl.hpp"

extern "C"
_Check_return_
long
__cdecl
atol(
    _In_z_ const char* const _String)
{
    return _atox_nosat_impl<int, char>(_String);
}

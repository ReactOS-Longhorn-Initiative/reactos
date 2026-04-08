/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strtoul
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "tcstox_impl.hpp"

extern "C"
_Check_return_
unsigned long
__cdecl
strtoul(
    _In_z_ const char* const _String,
    _Out_opt_ _Deref_post_z_ char** const _EndPtr,
    _In_ int const _Radix)
{
    return _tcstox_impl<unsigned long, char>(_String, _EndPtr, _Radix, false);
}

/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strtol
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "tcstox_impl.hpp"

extern "C"
_Check_return_
long
__cdecl
strtol(
    _In_z_ const char* const _String,
    _Out_opt_ _Deref_post_z_ char** const _EndPtr,
    _In_ int const _Radix)
{
    return _tcstox_impl<long, char>(_String, _EndPtr, _Radix, true);
}

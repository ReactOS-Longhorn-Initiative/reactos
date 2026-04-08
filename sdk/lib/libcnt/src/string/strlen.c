/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strlen
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#ifdef _MSC_VER
#pragma function(strlen)
#elif defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("strlen")))
#endif
_Check_return_
size_t
__cdecl
strlen(
    _In_z_ const char* const _Str)
{
    const char* p = _Str;

    while (*p != L'\0')
        p++;

    return (p - _Str);
}

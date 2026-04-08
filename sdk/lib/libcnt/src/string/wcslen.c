/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcslen
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#ifdef _MSC_VER
#pragma function(wcslen)
#elif defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("wcslen")))
#endif
_Check_return_
size_t
__cdecl
wcslen(
    _In_z_ const wchar_t* const _String)
{
    const wchar_t* p = _String;

    while (*p != L'\0')
        p++;

    return (p - _String);
}

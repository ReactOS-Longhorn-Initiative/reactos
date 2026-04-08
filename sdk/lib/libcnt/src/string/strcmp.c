/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strcmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("strcmp")))
#endif
_Check_return_
int
__cdecl
strcmp(
    _In_z_ const char* const _Str1,
    _In_z_ const char* const _Str2)
{
    const char* str1 = _Str1;
    const char* str2 = _Str2;

    while (*str1 == *str2)
    {
        if (*str1 == '\0')
            return 0;
        str1++;
        str2++;
    }

    return ((unsigned char)*str1 > (unsigned char)*str2) ? 1 : -1;
}

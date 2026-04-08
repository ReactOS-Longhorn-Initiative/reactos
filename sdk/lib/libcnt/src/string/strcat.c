/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strcat
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("strcat")))
#endif
char*
__cdecl
strcat(
    _Inout_z_ char* const _Destination,
    _In_z_ const char* const _Source)
{
    char* dst = _Destination;
    const char* src = _Source;

    while (*dst != '\0')
        dst++;

    while ((*dst++ = *src++) != '\0')
        ;

    return _Destination;
}

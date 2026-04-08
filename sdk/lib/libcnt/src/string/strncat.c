/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strncat
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("strncat")))
#endif
char*
__cdecl
strncat(
    _Inout_z_ char* const _Destination,
    _In_reads_or_z_(_Count) const char* const _Source,
    _In_ size_t const _Count)
{
    char* dst = _Destination;
    const char* src = _Source;
    size_t count = _Count;

    while (*dst != '\0')
        dst++;

    while ((count > 0) && (*src != '\0'))
    {
        *dst++ = *src++;
        count--;
    }

    *dst = '\0';

    return _Destination;
}

/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strcpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("strcpy")))
#endif
char*
__cdecl
strcpy(
    _Out_writes_z_(_String_length_(_Source) + 1) char* const _Destination,
    _In_z_ const char* const _Source)
{
    char* dst = _Destination;
    const char* src = _Source;

    while ((*dst++ = *src++) != '\0')
        ;

    return _Destination;
}

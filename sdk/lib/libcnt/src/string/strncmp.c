/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strncmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("strncmp")))
#endif
_Check_return_
int
__cdecl
strncmp(
    _In_reads_or_z_(_MaxCount) const char* const _String1,
    _In_reads_or_z_(_MaxCount) const char* const _String2,
    _In_ size_t const _MaxCount)
{
    const char* str1 = _String1;
    const char* str2 = _String2;
    size_t remaining = _MaxCount;

    while (remaining > 0)
    {
        if (*str1 != *str2)
            return (unsigned char)*str1 - (unsigned char)*str2;
        if (*str1 == '\0')
            return 0;
        str1++;
        str2++;
        remaining--;
    }

    return 0;
}

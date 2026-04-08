/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strnlen
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#pragma warning(suppress: 28196) // The analyzer cannot prove the postcondition through the loop
#if defined(__clang__)
__attribute__((no_builtin("strnlen")))
#endif
_Check_return_
_When_(_MaxCount > _String_length_(_String),
       _Post_satisfies_(return == _String_length_(_String)))
_When_(_MaxCount <= _String_length_(_String),
       _Post_satisfies_(return == _MaxCount))
size_t
__cdecl
strnlen(
    _In_reads_or_z_(_MaxCount) const char* const _String,
    _In_ size_t const _MaxCount)
{
    size_t count = 0;

    while ((count < _MaxCount) && (_String[count] != '\0'))
        count++;

    return count;
}

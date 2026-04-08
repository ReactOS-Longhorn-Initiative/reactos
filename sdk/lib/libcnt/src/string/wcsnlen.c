/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcsnlen
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#pragma warning(suppress: 28196) // The analyzer cannot prove the postcondition through the loop
#if defined(__clang__)
__attribute__((no_builtin("wcsnlen")))
#endif
_Check_return_
_When_(
    _MaxCount > _String_length_(_Source),
    _Post_satisfies_(return == _String_length_(_Source))
)
_When_(
    _MaxCount <= _String_length_(_Source),
    _Post_satisfies_(return == _MaxCount)
)
size_t
__cdecl
wcsnlen(
    _In_reads_or_z_(_MaxCount) const wchar_t* const _Source,
    _In_ size_t const _MaxCount)
{
    size_t count = 0;

    while ((count < _MaxCount) && (_Source[count] != L'\0'))
        count++;

    return count;
}

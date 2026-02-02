/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcschr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

_NODISCARD
_Check_return_
_When_(return != NULL, _Ret_range_(Str, Str + _String_length_(Str) - 1))
wchar_t*
__cdecl
wcschr(
    _In_z_ const wchar_t* const Str,
    _In_ wchar_t const Ch)
{
    const wchar_t* ptr = Str;

    while (*ptr)
    {
        if (*ptr == Ch)
            return (wchar_t*)ptr;
        ptr++;
    }

    if (Ch == L'\0')
        return (wchar_t*)ptr;

    return NULL;
}

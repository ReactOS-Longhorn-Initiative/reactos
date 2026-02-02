/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcsrchr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

_NODISCARD
_Check_return_
wchar_t*
__cdecl
wcsrchr(
    _In_z_ const wchar_t* const Str,
    _In_ wchar_t const Ch)
{
    const wchar_t* last_occurrence = NULL;
    const wchar_t* ptr = Str;

    while (*ptr)
    {
        if (*ptr == Ch)
            last_occurrence = ptr;
        ptr++;
    }

    if (Ch == L'\0')
        return (wchar_t*)ptr;

    return (wchar_t*)last_occurrence;
}

/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcsstr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

_NODISCARD
_Check_return_
_Ret_maybenull_
_When_(return != NULL, _Ret_range_(Str, Str + _String_length_(Str) - 1))
wchar_t*
__cdecl
wcsstr(
    _In_z_ const wchar_t* const Str,
    _In_z_ const wchar_t* const SubStr)
{
    const wchar_t* candidate = Str;

    /* Keep searching until the string ends */
    while (*candidate != '\0')
    {
        /* This is a candidate, check if it matches */
        size_t i = 0;
        while ((candidate[i] == SubStr[i]) && (SubStr[i] != '\0'))
        {
            i++;
        }

        /* If we reached the end of the substring, we found a match */
        if (SubStr[i] == L'\0')
            return (wchar_t*)candidate;

        candidate++;
    }

    return NULL;
}

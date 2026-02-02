/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strstr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

_NODISCARD
_Check_return_
_Ret_maybenull_
char*
__cdecl
strstr(
    _In_z_ const char* const Str,
    _In_z_ const char* const SubStr)
{
    const char* candidate = Str;

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
        if (SubStr[i] == '\0')
            return (char*)candidate;

        candidate++;
    }

    return NULL;
}

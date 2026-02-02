/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strrchr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

_NODISCARD
_Check_return_
char*
__cdecl strrchr(
    _In_z_ const char* const Str,
    _In_ int const Ch)
{
    const char* last_occurrence = NULL;
    const char* ptr = Str;
    char chr = (char)Ch;

    while (*ptr)
    {
        if (*ptr == chr)
            last_occurrence = ptr;
        ptr++;
    }

    if (chr == '\0')
        return (char*)ptr;

    return (char*)last_occurrence;
}

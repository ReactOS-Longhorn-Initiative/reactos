/*
 * PROJECT:     ReactOS vcruntime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strchr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <vcruntime_string.h>

_NODISCARD
_Check_return_
char*
__cdecl
strchr(
    _In_z_ const char* const Str,
    _In_ int const Val)
{
    const char* ptr = Str;
    char ch = (char)Val;

    while (*ptr)
    {
        if (*ptr == ch)
            return (char*)ptr;
        ptr++;
    }

    if (ch == '\0')
        return (char*)ptr;

    return NULL;
}

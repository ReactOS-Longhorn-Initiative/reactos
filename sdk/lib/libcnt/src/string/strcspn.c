/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strcspn
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
size_t
__cdecl
strcspn(
    _In_z_ const char* const _String,
    _In_z_ const char* const _Control)
{
    const char* str = _String;

    while (*str != '\0')
    {
        const char* ctrl = _Control;
        while (*ctrl != '\0')
        {
            if (*ctrl == *str)
                return (size_t)(str - _String);
            ctrl++;
        }

        str++;
    }

    return (size_t)(str - _String);
}

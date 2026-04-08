/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcspbrk
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

_Check_return_
wchar_t*
__cdecl
wcspbrk(
    _In_z_ const wchar_t* const _String,
    _In_z_ const wchar_t* const _Control)
{
    const wchar_t* str = _String;

    while (*str != L'\0')
    {
        const wchar_t* ctrl = _Control;
        while (*ctrl != L'\0')
        {
            if (*ctrl == *str)
                return (wchar_t*)str;
            ctrl++;
        }

        str++;
    }

    return NULL;
}

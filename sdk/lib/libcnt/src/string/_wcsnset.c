/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wcsnset
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

void __cdecl _invalid_parameter_noinfo(void);

wchar_t*
__cdecl
_wcsnset(
    _Inout_updates_z_(_MaxCount) wchar_t* _String,
    _In_ wchar_t _Value,
    _In_ size_t _MaxCount)
{
    wchar_t *p = _String;
    size_t remaining;

    if (_String == NULL)
    {
        _invalid_parameter_noinfo();
        return NULL;
    }

    /* Fill up to _MaxCount characters while scanning for null terminator */
    remaining = _MaxCount;
    while (remaining != 0)
    {
        if (*p == '\0')
            return 0;

        *p++ = _Value;
        remaining--;
    }

    return _String;
}

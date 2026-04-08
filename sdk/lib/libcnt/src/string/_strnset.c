/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strnset
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

void __cdecl _invalid_parameter_noinfo(void);

char*
__cdecl
_strnset(
    _Inout_updates_z_(_MaxCount) char* _String,
    _In_ int _Value,
    _In_ size_t _MaxCount)
{
    char *p = _String;
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

        *p++ = (char)_Value;
        remaining--;
    }

    return _String;
}

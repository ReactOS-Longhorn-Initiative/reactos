/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strnset_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>
#include <minmax.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
_strnset_s(
    _Inout_updates_z_(_SizeInElements) char* const _String,
    _In_ size_t const _SizeInElements,
    _In_ int const _Value,
    _In_ size_t const _MaxCount)
{
    char *p = _String;
    size_t remaining;

    if ((_String == NULL) || (_SizeInElements == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Fill up to min(_MaxCount, _SizeInElements - 1) characters while scanning for null terminator */
    remaining = min(_MaxCount, _SizeInElements - 1);
    while (remaining != 0)
    {
        if (*p == '\0')
            return 0;

        *p++ = (char)_Value;
        remaining--;
    }

    /* No more characters to write. Check if we have space for a null terminator */
    if (_MaxCount >= _SizeInElements)
    {
        _String[0] = '\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    return 0;
}

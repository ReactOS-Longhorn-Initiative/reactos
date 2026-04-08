/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strset_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
_strset_s(
    _Inout_updates_z_(_SizeInElements) char* const _String,
    _In_ size_t const _SizeInElements,
    _In_ int const _Value)
{
    char *p = _String;
    size_t remaining;

    if ((_String == NULL) || (_SizeInElements == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Fill characters while scanning for null terminator */
    remaining = _SizeInElements - 1;
    while (remaining != 0)
    {
        if (*p == '\0')
            return 0;

        *p++ = (char)_Value;
        remaining--;
    }

    /* Check last element for null terminator */
    if (*p != '\0')
    {
        _String[0] = '\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    return 0;
}

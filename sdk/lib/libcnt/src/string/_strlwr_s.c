/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strlwr_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
_strlwr_s(
    _Inout_updates_z_(_Size) char* const _String,
    _In_ size_t const _Size)
{
    char *p = _String;
    size_t remaining = _Size;

    if ((_String == NULL) || (_Size == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Verify that there is a null terminator within _Size bytes */
    while (remaining != 0)
    {
        if (*p == '\0')
            break;
        p++;
        remaining--;
    }

    if (remaining == 0)
    {
        _String[0] = '\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Convert to lowercase */
    for (p = _String; *p; p++)
    {
        if ((*p >= 'A') && (*p <= 'Z'))
        {
            *p += 'a' - 'A';
        }
    }

    return 0;
}

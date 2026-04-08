/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wcslwr_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
_wcslwr_s(
    _Inout_updates_z_(_SizeInWords) wchar_t* const _String,
    _In_ size_t const _SizeInWords)
{
    wchar_t *p = _String;
    size_t remaining = _SizeInWords;

    if ((_String == NULL) || (_SizeInWords == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Verify that there is a null terminator within _SizeInWords */
    while (remaining != 0)
    {
        if (*p == L'\0')
            break;
        p++;
        remaining--;
    }

    if (remaining == 0)
    {
        _String[0] = L'\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Convert to lowercase */
    for (p = _String; *p; p++)
    {
        if ((*p >= L'A') && (*p <= L'Z'))
        {
            *p += L'a' - L'A';
        }
    }

    return 0;
}

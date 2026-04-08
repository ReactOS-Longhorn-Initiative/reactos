/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcscat_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
wcscat_s(
    _Inout_updates_z_(_SizeInWords) wchar_t* const _Destination,
    _In_ rsize_t const _SizeInWords,
    _In_z_ const wchar_t* const _Source)
{
    if ((_Destination == NULL) || (_SizeInWords == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    if (_Source == NULL)
    {
        _Destination[0] = L'\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Find the end of the existing string in the destination */
    wchar_t* dst = _Destination;
    const wchar_t* src = _Source;
    rsize_t remaining = _SizeInWords;

    while ((remaining) && (*dst != L'\0'))
    {
        dst++;
        remaining--;
    }

    if (remaining == 0)
    {
        /* Destination is not null-terminated within buffer */
        _Destination[0] = L'\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    while (remaining--)
    {
        if ((*dst++ = *src++) == L'\0')
            return 0;
    }

    /* Buffer too small */
    _Destination[0] = L'\0';
    _invalid_parameter_noinfo();
    return ERANGE;
}

/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strncat_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
strncat_s(
    _Inout_updates_z_(_SizeInBytes) char* const _Destination,
    _In_ rsize_t const _SizeInBytes,
    _In_reads_or_z_(_MaxCount) const char* const _Source,
    _In_ rsize_t const _MaxCount)
{
    if ((_Destination == NULL) || (_SizeInBytes == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    if (_Source == NULL)
    {
        _Destination[0] = '\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    /* Find the end of the existing string in the destination */
    char* dst = _Destination;
    const char* src = _Source;
    rsize_t remaining = _SizeInBytes;

    while ((remaining) && (*dst != '\0'))
    {
        dst++;
        remaining--;
    }

    if (remaining == 0)
    {
        /* Destination is not null-terminated within buffer */
        _Destination[0] = '\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    if (_MaxCount == _TRUNCATE)
    {
        while (remaining > 1)
        {
            if ((*dst = *src) == '\0')
                return 0;
            dst++;
            src++;
            remaining--;
        }
        *dst = '\0';
        return (*src == '\0') ? 0 : STRUNCATE;
    }

    rsize_t count = (_MaxCount < remaining) ? _MaxCount : remaining - 1;

    while (count > 0)
    {
        if ((*dst = *src) == '\0')
            return 0;
        dst++;
        src++;
        count--;
    }

    if (remaining > _MaxCount)
    {
        /* Count exhausted, still have room for null */
        *dst = '\0';
        return 0;
    }

    if (*src == '\0')
    {
        *dst = '\0';
        return 0;
    }

    /* Buffer too small */
    _Destination[0] = '\0';
    _invalid_parameter_noinfo();
    return ERANGE;
}

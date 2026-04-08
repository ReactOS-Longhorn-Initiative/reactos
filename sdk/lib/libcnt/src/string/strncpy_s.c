/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strncpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
strncpy_s(
    _Out_writes_z_(_SizeInBytes) char* const _Destination,
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

    char* dst = _Destination;
    const char* src = _Source;
    rsize_t remaining = _SizeInBytes;

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
        /* No room for more chars - truncate */
        _Destination[_SizeInBytes - 1] = '\0';
        return STRUNCATE;
    }

    remaining = (_MaxCount < _SizeInBytes) ? _MaxCount : _SizeInBytes;

    while (remaining)
    {
        if ((*dst = *src) == '\0')
            return 0;
        dst++;
        src++;
        remaining--;
    }

    if (_SizeInBytes > _MaxCount)
    {
        /* Count exhausted, still have room for null */
        *dst = '\0';
        return 0;
    }

    /* Buffer too small */
    _Destination[0] = '\0';
    _invalid_parameter_noinfo();
    return ERANGE;
}

/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcsncpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
wcsncpy_s(
    _Out_writes_z_(_SizeInWords) wchar_t* const _Destination,
    _In_ rsize_t const _SizeInWords,
    _In_reads_or_z_(_MaxCount) const wchar_t* const _Source,
    _In_ rsize_t const _MaxCount)
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

    wchar_t* dst = _Destination;
    const wchar_t* src = _Source;
    rsize_t remaining = _SizeInWords;

    if (_MaxCount == _TRUNCATE)
    {
        while (remaining > 1)
        {
            if ((*dst = *src) == L'\0')
                return 0;
            dst++;
            src++;
            remaining--;
        }
        /* No room for more chars - truncate */
        _Destination[_SizeInWords - 1] = L'\0';
        return STRUNCATE;
    }

    remaining = (_MaxCount < _SizeInWords) ? _MaxCount : _SizeInWords;

    while (remaining)
    {
        if ((*dst = *src) == L'\0')
            return 0;
        dst++;
        src++;
        remaining--;
    }

    if (_SizeInWords > _MaxCount)
    {
        /* Count exhausted, still have room for null */
        *dst = L'\0';
        return 0;
    }

    /* Buffer too small */
    _Destination[0] = L'\0';
    _invalid_parameter_noinfo();
    return ERANGE;
}

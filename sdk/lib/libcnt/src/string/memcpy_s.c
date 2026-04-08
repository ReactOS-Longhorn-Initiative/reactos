/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of memcpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <crtdefs.h>
#include <errno.h>
#include <string.h>

void __cdecl _invalid_parameter_noinfo(void);

_Success_(return == 0)
_Check_return_opt_
errno_t
__cdecl
memcpy_s(
    _Out_writes_bytes_to_opt_(_DestinationSize, _SourceSize) void* const _Destination,
    _In_ rsize_t const _DestinationSize,
    _In_reads_bytes_opt_(_SourceSize) void const* const _Source,
    _In_ rsize_t const _SourceSize)
{
    if (_SourceSize == 0)
    {
        return 0;
    }

    if (_Destination == NULL)
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    if ((_Source == NULL) || (_DestinationSize < _SourceSize))
    {
        memset(_Destination, 0, _DestinationSize);
        _invalid_parameter_noinfo();

        if (_Source == NULL)
        {
            return EINVAL;
        }
        else
        {
            return ERANGE;
        }
    }

    memmove(_Destination, _Source, _SourceSize);
    return 0;
}

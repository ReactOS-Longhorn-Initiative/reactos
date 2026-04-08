/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of memmove_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <crtdefs.h>
#include <errno.h>
#include <string.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
memmove_s(
    _Out_writes_bytes_to_opt_(_DestinationSize, _SourceSize) void* const _Destination,
    _In_ rsize_t const _DestinationSize,
    _In_reads_bytes_opt_(_SourceSize) void const* const _Source,
    _In_ rsize_t const _SourceSize)
{
    if (_SourceSize == 0)
    {
        return 0;
    }

    if ((_Destination == NULL) || (_Source == NULL))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    if (_DestinationSize < _SourceSize)
    {
        _invalid_parameter_noinfo();
        return ERANGE;
    }

    memmove(_Destination, _Source, _SourceSize);
    return 0;
}

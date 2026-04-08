/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strncpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

#if defined(__GNUC__)
#pragma GCC optimize("no-tree-loop-distribute-patterns")
#endif

#if defined(__clang__)
__attribute__((no_builtin("strncpy")))
#endif
char*
__cdecl
strncpy(
    _Out_writes_(_Count) char* const _Destination,
    _In_reads_or_z_(_Count) const char* const _Source,
    _In_ size_t const _Count)
{
    size_t i, len;

    /* Find source length up to _Count */
    for (len = 0; (len < _Count) && (_Source[len] != '\0'); len++)
        ;

    /* Copy string data - handle overlap by direction */
    if ((_Destination <= _Source) || (_Destination >= _Source + len))
    {
        for (i = 0; i < len; i++)
            _Destination[i] = _Source[i];
    }
    else
    {
        for (i = len; i > 0; i--)
            _Destination[i - 1] = _Source[i - 1];
    }

    /* Pad remaining with nulls */
    for (i = len; i < _Count; i++)
        _Destination[i] = '\0';

    return _Destination;
}

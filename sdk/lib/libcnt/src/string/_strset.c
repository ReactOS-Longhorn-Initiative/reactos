/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strset
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

char*
__cdecl
_strset(
    _Inout_z_ char *_Destination,
    _In_ int _Value)
{
    char *p = _Destination;

    if (_Destination == NULL)
    {
        //_invalid_parameter_noinfo();
        return NULL;
    }

    while (*p != '\0')
    {
        *p++ = (char)_Value;
    }

    return _Destination;
}

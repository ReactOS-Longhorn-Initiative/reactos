/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _strupr
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

void
__cdecl
_invalid_parameter(
    _In_opt_z_ wchar_t const* const _Expression,
    _In_opt_z_ wchar_t const* const _FunctionName,
    _In_opt_z_ wchar_t const* const _FileName,
    _In_ unsigned int const _LineNumber,
    _In_ uintptr_t const _Reserved);

char*
__cdecl
_strupr(
    _Inout_z_ char* const _String)
{
    char ch, *p;

    if (_String == NULL)
    {
        _invalid_parameter(NULL, L"_strupr", _CRT_WIDE(__FILE__), __LINE__, 0);
        return NULL;
    }

    for (p = _String; *p; p++)
    {
        ch = *p;
        if ((ch >= 'a') && (ch <= 'z'))
        {
            *p += 'A' - 'a';
        }
    }

    return _String;
}

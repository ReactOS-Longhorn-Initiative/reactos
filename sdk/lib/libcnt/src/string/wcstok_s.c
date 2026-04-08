/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of wcstok_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

static
int
_is_delimiter(
    const wchar_t _Char,
    const wchar_t* const _Delimiter)
{
    const wchar_t* delim = _Delimiter;
    while (*delim != L'\0')
    {
        if (*delim == _Char)
            return 1;
        delim++;
    }
    return 0;
}

_Check_return_
wchar_t*
__cdecl
wcstok_s(
    _Inout_opt_z_ wchar_t* const _String,
    _In_z_ const wchar_t* const _Delimiter,
    _Inout_ _Deref_prepost_opt_z_ wchar_t** const _Context)
{
    wchar_t* str = (_String != NULL) ? _String : *_Context;
    wchar_t* token;

    /* Skip leading delimiters */
    while ((*str != L'\0') && _is_delimiter(*str, _Delimiter))
    {
        str++;
    }

    /* If we reached end of string, no more tokens */
    if (*str == L'\0')
    {
        *_Context = str;
        return NULL;
    }

    /* Start of token */
    token = str;

    /* Find end of token */
    while ((*str != L'\0') && !_is_delimiter(*str, _Delimiter))
    {
        str++;
    }

    /* Null-terminate the token if a delimiter was found */
    if (*str != L'\0')
    {
        *str = L'\0';
        str++;
    }

    *_Context = str;
    return token;
}

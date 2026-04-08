/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strtok_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>

static
int
_is_delimiter(
    const char _Char,
    const char* const _Delimiter)
{
    const char* delim = _Delimiter;
    while (*delim != '\0')
    {
        if (*delim == _Char)
            return 1;
        delim++;
    }
    return 0;
}

_Check_return_
char*
__cdecl
strtok_s(
    _Inout_opt_z_ char* const _String,
    _In_z_ const char* const _Delimiter,
    _Inout_ _Deref_prepost_opt_z_ char** const _Context)
{
    char* str = (_String != NULL) ? _String : *_Context;
    char* token;

    /* Skip leading delimiters */
    while ((*str != '\0') && _is_delimiter(*str, _Delimiter))
    {
        str++;
    }

    /* If we reached end of string, no more tokens */
    if (*str == '\0')
    {
        *_Context = str;
        return NULL;
    }

    /* Start of token */
    token = str;

    /* Find end of token */
    while ((*str != '\0') && !_is_delimiter(*str, _Delimiter))
    {
        str++;
    }

    /* Null-terminate the token if a delimiter was found */
    if (*str != '\0')
    {
        *str = '\0';
        str++;
    }

    *_Context = str;
    return token;
}

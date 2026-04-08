/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _makepath_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
_makepath_s(
    _Out_writes_z_(_BufferCount) char* const _Buffer,
    _In_ size_t const _BufferCount,
    _In_opt_z_ const char* const _Drive,
    _In_opt_z_ const char* const _Dir,
    _In_opt_z_ const char* const _Filename,
    _In_opt_z_ const char* const _Ext)
{
    char* dst;
    char* end;

    if ((_Buffer == NULL) || (_BufferCount == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    dst = _Buffer;
    end = _Buffer + _BufferCount;

    /* Drive */
    if ((_Drive != NULL) && (_Drive[0] != '\0'))
    {
        if (dst >= end) goto overflow;
        *dst++ = _Drive[0];
        if (_Drive[1] != ':')
        {
            if (dst >= end) goto overflow;
            *dst++ = ':';
        }
        else
        {
            if (dst >= end) goto overflow;
            *dst++ = ':';
        }
    }

    /* Directory */
    if ((_Dir != NULL) && (_Dir[0] != '\0'))
    {
        const char* p = _Dir;
        while (*p != '\0')
        {
            if (dst >= end) goto overflow;
            *dst++ = *p++;
        }

        /* Add trailing backslash if dir doesn't end with one */
        {
            char last = *(p - 1);
            if ((last != '\\') && (last != '/'))
            {
                if (dst >= end) goto overflow;
                *dst++ = '\\';
            }
        }
    }

    /* Filename */
    if ((_Filename != NULL) && (_Filename[0] != '\0'))
    {
        const char* p = _Filename;
        while (*p != '\0')
        {
            if (dst >= end) goto overflow;
            *dst++ = *p++;
        }
    }

    /* Extension */
    if ((_Ext != NULL) && (_Ext[0] != '\0'))
    {
        if (_Ext[0] != '.')
        {
            if (dst >= end) goto overflow;
            *dst++ = '.';
        }
        {
            const char* p = _Ext;
            while (*p != '\0')
            {
                if (dst >= end) goto overflow;
                *dst++ = *p++;
            }
        }
    }

    /* Null terminator */
    if (dst >= end) goto overflow;
    *dst = '\0';
    return 0;

overflow:
    _Buffer[0] = '\0';
    _invalid_parameter_noinfo();
    return ERANGE;
}

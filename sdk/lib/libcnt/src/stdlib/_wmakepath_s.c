/*
 * PROJECT:     NT C Runtime Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _wmakepath_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
_wmakepath_s(
    _Out_writes_z_(BufferCount) wchar_t* const Buffer,
    _In_ size_t const BufferCount,
    _In_opt_z_ const wchar_t* const Drive,
    _In_opt_z_ const wchar_t* const Dir,
    _In_opt_z_ const wchar_t* const Filename,
    _In_opt_z_ const wchar_t* const Ext)
{
    wchar_t* dst;
    wchar_t* end;

    if ((Buffer == NULL) || (BufferCount == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    dst = Buffer;
    end = Buffer + BufferCount;

    /* Drive */
    if ((Drive != NULL) && (Drive[0] != L'\0'))
    {
        if (dst >= end) goto overflow;
        *dst++ = Drive[0];
        if (dst >= end) goto overflow;
        *dst++ = L':';
    }

    /* Directory */
    if ((Dir != NULL) && (Dir[0] != L'\0'))
    {
        const wchar_t* p = Dir;
        while (*p != L'\0')
        {
            if (dst >= end) goto overflow;
            *dst++ = *p++;
        }

        /* Add trailing backslash if dir doesn't end with one */
        {
            wchar_t last = *(p - 1);
            if ((last != L'\\') && (last != L'/'))
            {
                if (dst >= end) goto overflow;
                *dst++ = L'\\';
            }
        }
    }

    /* Filename */
    if ((Filename != NULL) && (Filename[0] != L'\0'))
    {
        const wchar_t* p = Filename;
        while (*p != L'\0')
        {
            if (dst >= end) goto overflow;
            *dst++ = *p++;
        }
    }

    /* Extension */
    if ((Ext != NULL) && (Ext[0] != L'\0'))
    {
        if (Ext[0] != L'.')
        {
            if (dst >= end) goto overflow;
            *dst++ = L'.';
        }
        {
            const wchar_t* p = Ext;
            while (*p != L'\0')
            {
                if (dst >= end) goto overflow;
                *dst++ = *p++;
            }
        }
    }

    /* Null terminator */
    if (dst >= end) goto overflow;
    *dst = L'\0';
    return 0;

overflow:
    Buffer[0] = L'\0';
    _invalid_parameter_noinfo();
    return ERANGE;
}

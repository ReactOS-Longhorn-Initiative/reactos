/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Implementation of _controlfp_s (adapted from wine msvcrt/math.c)
 * COPYRIGHT:   Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffiths
 *              Copyright 2011 Piotr Caban
 *              Copyright 2011 Akihiro Sagawa
 */

#include <precomp.h>

/*********************************************************************
 *		wctob (MSVCRT.@)
 */
INT CDECL wctob(wint_t wchar)
{
    char out;
    BOOL error = FALSE;
    BOOL* perror;
    UINT codepage = get_locinfo()->lc_codepage;

    perror = (codepage != CP_UTF8 ? &error : NULL);

    if (!codepage) {
        if (wchar < 0xff)
            return (signed char)wchar;
        else
            return EOF;
    }
    else if (WideCharToMultiByte(codepage, 0, &wchar, 1, &out, 1, NULL, perror) && !error)
        return (INT)out;
    return EOF;
}

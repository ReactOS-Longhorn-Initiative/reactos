/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Implementation of _controlfp_s (adapted from wine msvcrt/math.c)
 * COPYRIGHT:   Copyright 1999 Alexandre Julliard
 *              Copyright 2000 Jon Griffiths
 *              Copyright 2010 Piotr Caban
 *              Copyright 2020 Charles Davis
 */

#include <precomp.h>

 /*********************************************************************
  *              btowc(MSVCRT.@)
  */
wint_t CDECL btowc(int c)
{
    unsigned char letter = c;
    wchar_t ret;

    if (c == EOF)
        return WEOF;
    if (!get_locinfo()->lc_codepage)
        return c & 255;
    if (!MultiByteToWideChar(get_locinfo()->lc_codepage,
        MB_ERR_INVALID_CHARS, (LPCSTR)&letter, 1, &ret, 1))
        return WEOF;

    return ret;
}

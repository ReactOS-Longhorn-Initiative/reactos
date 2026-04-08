/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswctype
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswctype)(wint_t c, wctype_t type);

int __cdecl expected_iswctype(wint_t c, wctype_t type)
{
    if ((c >= 0x00) && (c <= 0x08)) return _CONTROL;
    if (c == 0x09) return _BLANK | _CONTROL | _SPACE;
    if ((c >= 0x0A) && (c <= 0x0D)) return _CONTROL | _SPACE;
    if ((c >= 0x0E) && (c <= 0x1F)) return _CONTROL;
    if (c == 0x20) return _BLANK | _SPACE;
    if ((c >= 0x21) && (c <= 0x2F)) return _PUNCT;
    if ((c >= '0') && (c <= '9')) return _HEX | _DIGIT;
    if ((c >= 0x3A) && (c <= 0x40)) return _PUNCT;
    if ((c >= 'A') && (c <= 'F')) return 0x100 | _UPPER | _HEX;
    if ((c >= 'G') && (c <= 'Z')) return 0x100 | _UPPER;
    if ((c >= 0x5B) && (c <= 0x60)) return _PUNCT;
    if ((c >= 'a') && (c <= 'f')) return 0x100 | _LOWER | _HEX;
    if ((c >= 'g') && (c <= 'z')) return 0x100 | _LOWER;
    if ((c >= 0x7B) && (c <= 0x7E)) return _PUNCT;
    if ((c >= 0x7F) && (c <= 0x9F)) return _CONTROL;
    if (c == 0xA0) return _BLANK | _SPACE;
    if ((c >= 0xA1) && (c <= 0xB1)) return _PUNCT;
    if ((c >= 0xB2) && (c <= 0xB3)) return _PUNCT | _DIGIT;
    if ((c >= 0xB4) && (c <= 0xB8)) return _PUNCT;
    if (c == 0xB9) return _PUNCT | _DIGIT;
    if ((c >= 0xBA) && (c <= 0xBF)) return _PUNCT;
    if ((c >= 0xC0) && (c <= 0xD6)) return 0x100 | _UPPER;
    if (c == 0xD7) return _PUNCT;
    if ((c >= 0xD8) && (c <= 0xDE)) return 0x100 | _UPPER;
    if ((c >= 0xDF) && (c <= 0xF6)) return 0x100 | _LOWER;
    if (c == 0xF7) return _PUNCT;
    if ((c >= 0xF8) && (c <= 0xFF)) return 0x100 | _LOWER;
    return 0;
}

START_TEST(iswctype)
{
    PFN_iswctype p_iswctype = load_function("iswctype");

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswctype(c, 0xFFFF);
        int ret = p_iswctype(c, 0xFFFF);
        ok(ret == expected, "iswctype(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

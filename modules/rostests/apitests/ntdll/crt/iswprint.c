/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswprint
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswprint)(wint_t c);

int __cdecl expected_iswprint(wint_t c)
{
    if (c == 0x09) return _BLANK;
    if (c == 0x20) return _BLANK;
    if ((c >= 0x21) && (c <= 0x2F)) return _PUNCT;
    if ((c >= '0') && (c <= '9')) return _DIGIT;
    if ((c >= 0x3A) && (c <= 0x40)) return _PUNCT;
    if ((c >= 'A') && (c <= 'F')) return 0x100 | _UPPER;
    if ((c >= 'G') && (c <= 'Z')) return 0x100 | _UPPER;
    if ((c >= 0x5B) && (c <= 0x60)) return _PUNCT;
    if ((c >= 'a') && (c <= 'f')) return 0x100 | _LOWER;
    if ((c >= 'g') && (c <= 'z')) return 0x100 | _LOWER;
    if ((c >= 0x7B) && (c <= 0x7E)) return _PUNCT;
    if (c == 0xA0) return _BLANK;
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

START_TEST(iswprint)
{
    PFN_iswprint p_iswprint = load_function("iswprint");

    if (p_iswprint == NULL)
    {
        skip("Function iswprint not available\n");
        return;
    }

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswprint(c);
        int ret = p_iswprint(c);
        ok(ret == expected, "iswprint(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for isprint
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_isprint)(int c);

int __cdecl expected_isprint(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= 'A') && (uc <= 'Z')) return _UPPER;
    if ((uc >= 'a') && (uc <= 'z')) return _LOWER;
    if ((uc >= '0') && (uc <= '9')) return _DIGIT;
    if ((uc >= '\x21') && (uc <= '\x2F')) return _PUNCT;
    if ((uc >= '\x3A') && (uc <= '\x40')) return _PUNCT;
    if ((uc >= '\x5B') && (uc <= '\x60')) return _PUNCT;
    if ((uc >= '\x7B') && (uc <= '\x7E')) return _PUNCT;
    if (uc == '\x20') return _BLANK;
    return 0;
}

START_TEST(isprint)
{
    PFN_isprint p_isprint = load_function("isprint");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_isprint(c);
        int ret = p_isprint(c);
        ok(ret == expected, "isprint(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

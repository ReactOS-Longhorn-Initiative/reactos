/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for isgraph
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_isgraph)(int c);

int __cdecl expected_isgraph(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= 'A') && (uc <= 'Z')) return _UPPER;
    if ((uc >= 'a') && (uc <= 'z')) return _LOWER;
    if ((uc >= '0') && (uc <= '9')) return _DIGIT;
    if ((uc >= '\x21') && (uc <= '\x2F')) return _PUNCT;
    if ((uc >= '\x3A') && (uc <= '\x40')) return _PUNCT;
    if ((uc >= '\x5B') && (uc <= '\x60')) return _PUNCT;
    if ((uc >= '\x7B') && (uc <= '\x7E')) return _PUNCT;
    return 0;
}

START_TEST(isgraph)
{
    PFN_isgraph p_isgraph = load_function("isgraph");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_isgraph(c);
        int ret = p_isgraph(c);
        ok(ret == expected, "isgraph(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

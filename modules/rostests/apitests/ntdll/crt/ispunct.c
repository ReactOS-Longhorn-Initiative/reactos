/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for ispunct
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_ispunct)(int c);

int __cdecl expected_ispunct(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= '\x21') && (uc <= '\x2F')) return _PUNCT;
    if ((uc >= '\x3A') && (uc <= '\x40')) return _PUNCT;
    if ((uc >= '\x5B') && (uc <= '\x60')) return _PUNCT;
    if ((uc >= '\x7B') && (uc <= '\x7E')) return _PUNCT;
    return 0;
}

START_TEST(ispunct)
{
    PFN_ispunct p_ispunct = load_function("ispunct");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_ispunct(c);
        int ret = p_ispunct(c);
        ok(ret == expected, "ispunct(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

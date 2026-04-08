/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for isalpha
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_isalpha)(int c);

int __cdecl expected_isalpha(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= 'A') && (uc <= 'Z')) return _UPPER;
    if ((uc >= 'a') && (uc <= 'z')) return _LOWER;
    return 0;
}

START_TEST(isalpha)
{
    PFN_isalpha p_isalpha = load_function("isalpha");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_isalpha(c);
        int ret = p_isalpha(c);
        ok(ret == expected, "isalpha(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

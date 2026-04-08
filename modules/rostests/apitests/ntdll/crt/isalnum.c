/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for isalnum
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_isalnum)(int c);

int __cdecl expected_isalnum(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= 'A') && (uc <= 'Z')) return _UPPER;
    if ((uc >= 'a') && (uc <= 'z')) return _LOWER;
    if ((uc >= '0') && (uc <= '9')) return _DIGIT;
    return 0;
}

START_TEST(isalnum)
{
    PFN_isalnum p_isalnum = load_function("isalnum");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_isalnum(c);
        int ret = p_isalnum(c);
        ok(ret == expected, "isalnum(0x%lx) = %d, expected %d\n", c, ret, expected);
    }
}

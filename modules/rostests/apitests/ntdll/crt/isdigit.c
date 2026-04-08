/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for isdigit
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_isdigit)(int c);

int __cdecl expected_isdigit(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= '0') && (uc <= '9')) return _DIGIT;
    return 0;
}

START_TEST(isdigit)
{
    PFN_isdigit p_isdigit = load_function("isdigit");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_isdigit(c);
        int ret = p_isdigit(c);
        ok(ret == expected, "isdigit(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

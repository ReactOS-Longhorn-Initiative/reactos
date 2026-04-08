/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for isxdigit
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_isxdigit)(int c);

int __cdecl expected_isxdigit(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= '0') && (uc <= '9')) return _HEX;
    if ((uc >= 'A') && (uc <= 'F')) return _HEX;
    if ((uc >= 'a') && (uc <= 'f')) return _HEX;
    return 0;
}

START_TEST(isxdigit)
{
    PFN_isxdigit p_isxdigit = load_function("isxdigit");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_isxdigit(c);
        int ret = p_isxdigit(c);
        ok(ret == expected, "isxdigit(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

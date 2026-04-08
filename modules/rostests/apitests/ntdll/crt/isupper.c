/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for isupper
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_isupper)(int c);

int __cdecl expected_isupper(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= 'A') && (uc <= 'Z')) return _UPPER;
    return 0;
}

START_TEST(isupper)
{
    PFN_isupper p_isupper = load_function("isupper");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_isupper(c);
        int ret = p_isupper(c);
        ok(ret == expected, "isupper(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

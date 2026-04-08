/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for isspace
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_isspace)(int c);

int __cdecl expected_isspace(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= '\x09') && (uc <= '\x0D')) return _SPACE;
    if (uc == '\x20') return _SPACE;
    return 0;
}

START_TEST(isspace)
{
    PFN_isspace p_isspace = load_function("isspace");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_isspace(c);
        int ret = p_isspace(c);
        ok(ret == expected, "isspace(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

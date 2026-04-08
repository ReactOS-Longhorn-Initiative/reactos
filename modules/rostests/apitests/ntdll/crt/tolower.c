/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for tolower
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_tolower)(int c);

int __cdecl expected_tolower(int c)
{
    if (((char)c >= 'A') && ((char)c <= 'Z'))
        return c + ('a' - 'A');
    return c;
}

START_TEST(tolower)
{
    PFN_tolower p_tolower = load_function("tolower");

    for (int c = -0x17f; c <= 0xFFFF; c++)
    {
        int expected = expected_tolower(c);
        int ret = p_tolower(c);
        ok(ret == expected, "tolower(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }

    ok_eq_int(p_tolower(0x10000 + 'A'), 0x10000 + 'a');
    ok_eq_int(p_tolower(0x800000 + 'A'), 0x800000 + 'a');
}

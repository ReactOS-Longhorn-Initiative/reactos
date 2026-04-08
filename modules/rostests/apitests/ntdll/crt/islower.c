/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for islower
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_islower)(int c);

int __cdecl expected_islower(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= 'a') && (uc <= 'z')) return _LOWER;
    return 0;
}

START_TEST(islower)
{
    PFN_islower p_islower = load_function("islower");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_islower(c);
        int ret = p_islower(c);
        ok(ret == expected, "islower(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

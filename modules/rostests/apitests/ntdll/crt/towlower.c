/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for towlower
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef wint_t (*PFN_towlower)(wint_t c);

wint_t __cdecl expected_towlower(wint_t c)
{
    if ((c >= 'A') && (c <= 'Z')) return c + ('a' - 'A');
    if ((c >= 0xC0u) && (c <= 0xD6u)) return c + 0x20;
    if ((c >= 0xD8u) && (c <= 0xDEu)) return c + 0x20;
    return c;
}

START_TEST(towlower)
{
    PFN_towlower p_towlower = load_function("towlower");

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        wint_t expected = expected_towlower(c);
        wint_t ret = p_towlower(c);
        ok(ret == expected, "towlower(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

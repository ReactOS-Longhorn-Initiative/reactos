/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for toupper
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_toupper)(int c);

int __cdecl expected_toupper(int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= 'a') && (uc <= 'z')) return uc - ('a' - 'A');
    if ((uc >= 0xE0) && (uc <= 0xF6)) return uc - 0x20;
    if ((uc >= 0xF8) && (uc <= 0xFE)) return uc - 0x20;
    if (uc == 0xFF) return 0x9F;
    if (uc == 0x9A) return 0x8A;
    if (uc == 0x9C) return 0x8C;
    if (uc == 0x9E) return 0x8E;
    return uc;
}

START_TEST(toupper)
{
    PFN_toupper p_toupper = load_function("toupper");

    for (int c = -0x17f; c <= 0xFFFF; c++)
    {
        int expected = expected_toupper(c);
        int ret = p_toupper(c);
        ok(ret == expected, "toupper(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }

    ok_eq_int(p_toupper(0x10000 + 'a'), 'A');
    ok_eq_int(p_toupper(0x80000 + 'a'), 'A');
}

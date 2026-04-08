/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iscntrl
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iscntrl)(int c);

int __cdecl expected_iscntrl(unsigned int c)
{
    unsigned char uc = (unsigned char)c;
    if ((uc >= '\x00') && (uc <= '\x1F')) return _CONTROL;
    if (uc == '\x7F') return _CONTROL;
    return 0;
}

START_TEST(iscntrl)
{
    PFN_iscntrl p_iscntrl = load_function("iscntrl");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected_iscntrl(c);
        int ret = p_iscntrl(c);
        ok(ret == expected, "iscntrl(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

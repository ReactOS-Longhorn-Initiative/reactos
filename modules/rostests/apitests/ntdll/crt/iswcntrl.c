/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswcntrl
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswcntrl)(wint_t c);

int __cdecl expected_iswcntrl(wint_t c)
{
    if ((c >= '\x00') && (c <= '\x1F')) return _CONTROL;
    if (c == '\x7F') return _CONTROL;
    return 0;
}

START_TEST(iswcntrl)
{
    PFN_iswcntrl p_iswcntrl = load_function("iswcntrl");
    if (!p_iswcntrl)
    {
        skip("iswcntrl not available\n");
        return;
    }

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswcntrl(c);
        int ret = p_iswcntrl(c);
        ok(ret == expected, "iswcntrl(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

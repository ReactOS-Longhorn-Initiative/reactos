/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswspace
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswspace)(wint_t c);

int __cdecl expected_iswspace(wint_t c)
{
    if (c == 0x09) return _SPACE;
    if ((c >= 0x0A) && (c <= 0x0D)) return _SPACE;
    if (c == 0x20) return _SPACE;
    if (c == 0xA0) return _SPACE;
    return 0;
}

START_TEST(iswspace)
{
    PFN_iswspace p_iswspace = load_function("iswspace");

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswspace(c);
        int ret = p_iswspace(c);
        ok(ret == expected, "iswspace(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

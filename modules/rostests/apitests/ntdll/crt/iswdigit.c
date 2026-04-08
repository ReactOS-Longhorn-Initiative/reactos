/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswdigit
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswdigit)(wint_t c);

int __cdecl expected_iswdigit(wint_t c)
{
    if ((c >= '0') && (c <= '9')) return _DIGIT;
    if ((c >= 0xB2u) && (c <= 0xB3u)) return _DIGIT;
    if (c == 0xB9u) return _DIGIT;
    return 0;
}

START_TEST(iswdigit)
{
    PFN_iswdigit p_iswdigit = load_function("iswdigit");

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswdigit(c);
        int ret = p_iswdigit(c);
        ok(ret == expected, "iswdigit(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

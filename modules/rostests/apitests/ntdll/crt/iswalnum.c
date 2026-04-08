/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswalnum
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswalnum)(wint_t c);

int __cdecl expected_iswalnum(wint_t c)
{
    if ((c >= 'A') && (c <= 'Z')) return 0x0100 | _UPPER;
    if ((c >= 'a') && (c <= 'z')) return 0x0100 | _LOWER;
    if ((c >= '0') && (c <= '9')) return _DIGIT;
    if ((c >= 0xB2u) && (c <= 0xB3u)) return _DIGIT;
    if (c == 0xB9u) return _DIGIT;
    if ((c >= 0xC0u) && (c <= 0xCFu)) return 0x0100 | _UPPER;
    if ((c >= 0xD0u) && (c <= 0xD6u)) return 0x0100 | _UPPER;
    if ((c >= 0xD8u) && (c <= 0xDEu)) return 0x0100 | _UPPER;
    if ((c >= 0xDFu) && (c <= 0xF6u)) return 0x0100 | _LOWER;
    if ((c >= 0xF8u) && (c <= 0xFFu)) return 0x0100 | _LOWER;
    return 0;
}

START_TEST(iswalnum)
{
    PFN_iswalnum p_iswalnum = load_function("iswalnum");

    if (p_iswalnum == NULL)
    {
        skip("Function iswalnum not available\n");
        return;
    }

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswalnum(c);
        int ret = p_iswalnum(c);
        ok(ret == expected, "iswalnum(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

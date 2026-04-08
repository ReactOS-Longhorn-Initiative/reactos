/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswalpha
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswalpha)(wint_t c);

int __cdecl expected_iswalpha(wint_t c)
{
    if ((c >= 'A') && (c <= 'Z')) return 0x0100 | _UPPER;
    if ((c >= 'a') && (c <= 'z')) return 0x0100 | _LOWER;
    if ((c >= 0xC0u) && (c <= 0xCFu)) return 0x0100 | _UPPER;
    if ((c >= 0xD0u) && (c <= 0xD6u)) return 0x0100 | _UPPER;
    if ((c >= 0xD8u) && (c <= 0xDEu)) return 0x0100 | _UPPER;
    if ((c >= 0xDFu) && (c <= 0xF6u)) return 0x0100 | _LOWER;
    if ((c >= 0xF8u) && (c <= 0xFFu)) return 0x0100 | _LOWER;
    return 0;
}

START_TEST(iswalpha)
{
    PFN_iswalpha p_iswalpha = load_function("iswalpha");

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswalpha(c);
        int ret = p_iswalpha(c);
        ok(ret == expected, "iswalpha(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

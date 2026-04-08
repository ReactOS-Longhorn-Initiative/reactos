/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswlower
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswlower)(wint_t c);

int __cdecl expected_iswlower(wint_t c)
{
    if ((c >= 'a') && (c <= 'z')) return _LOWER;
    if ((c >= 0xDFu) && (c <= 0xF6u)) return _LOWER;
    if ((c >= 0xF8u) && (c <= 0xFFu)) return _LOWER;
    return 0;
}

START_TEST(iswlower)
{
    PFN_iswlower p_iswlower = load_function("iswlower");

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswlower(c);
        int ret = p_iswlower(c);
        ok(ret == expected, "iswlower(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

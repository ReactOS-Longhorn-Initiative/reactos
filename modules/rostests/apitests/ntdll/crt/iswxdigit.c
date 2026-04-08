/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswxdigit
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswxdigit)(wint_t c);

int __cdecl expected_iswxdigit(wint_t c)
{
    if ((c >= '0') && (c <= '9')) return _HEX;
    if ((c >= 'A') && (c <= 'F')) return _HEX;
    if ((c >= 'a') && (c <= 'f')) return _HEX;
    return 0;
}

START_TEST(iswxdigit)
{
    PFN_iswxdigit p_iswxdigit = load_function("iswxdigit");

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswxdigit(c);
        int ret = p_iswxdigit(c);
        ok(ret == expected, "iswxdigit(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for iswascii
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>

typedef int (*PFN_iswascii)(wint_t c);

int __cdecl expected_iswascii(wint_t c)
{
    if ((c >= 0x00) && (c <= 0x7F)) return 1;
    return 0;
}

START_TEST(iswascii)
{
    PFN_iswascii p_iswascii = load_function("iswascii");

    if (p_iswascii == NULL)
    {
        skip("Function iswascii not available\n");
        return;
    }

    for (unsigned int c = 0; c <= 0xFFFF; c++)
    {
        int expected = expected_iswascii(c);
        int ret = p_iswascii(c);
        ok(ret == expected, "iswascii(0x%x) = 0x%x, expected 0x%x\n", c, ret, expected);
    }
}

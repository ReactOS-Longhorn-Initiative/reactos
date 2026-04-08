/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for __isascii
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"

typedef int (*PFN___isascii)(int c);

START_TEST(__isascii)
{
    PFN___isascii p__isascii = load_function("__isascii");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = (c >= 0 && c <= 0x7f) ? 1 : 0;
        int ret = p__isascii(c);
        ok(ret == expected, "__isascii(0x%lx) = %d, expected %d\n", c, ret, expected);
    }
}

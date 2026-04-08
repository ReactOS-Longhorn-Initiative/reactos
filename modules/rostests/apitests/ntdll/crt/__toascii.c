/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for __toascii
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>
#include <stdlib.h>

typedef int (*PFN___toascii)(int c);

int expected___toascii(int c)
{
    return (c & 0x7f);
}

START_TEST(__toascii)
{
    PFN___toascii p__toascii = load_function("__toascii");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected___toascii(c);
        int ret = p__toascii(c);
        ok(ret == expected, "__toascii(0x%lx) = %d, expected %d\n", c, ret, expected);
    }
}

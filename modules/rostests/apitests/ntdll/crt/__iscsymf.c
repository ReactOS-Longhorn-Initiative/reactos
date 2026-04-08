/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for __iscsymf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>
#include <stdlib.h>

typedef int (*PFN___iscsymf)(int c);

int expected___iscsymf(int c)
{
    if (isalpha(c & 0xFF)) return 1;
    if (c == '_') return 1;
    return 0;
}

START_TEST(__iscsymf)
{
    PFN___iscsymf p__iscsymf = load_function("__iscsymf");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected___iscsymf(c);
        int ret = p__iscsymf(c);
        ok(ret == expected, "__iscsymf(0x%lx) = %d, expected %d\n", c, ret, expected);
    }
}

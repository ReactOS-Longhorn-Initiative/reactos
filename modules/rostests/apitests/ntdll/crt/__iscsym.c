/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for __iscsym
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <ctype.h>
#include <stdlib.h>

typedef int (*PFN___iscsym)(int c);

int expected___iscsym(int c)
{
    if (isalnum(c & 0xFF)) return 1;
    if ((c & 0xFF) == '_') return 1;
    return 0;
}

START_TEST(__iscsym)
{
    PFN___iscsym p__iscsym = load_function("__iscsym");

    for (int c = -0x17f; c <= 0x17f; c++)
    {
        int expected = expected___iscsym(c);
        int ret = p__iscsym(c);
        ok(ret == expected, "__iscsym(0x%lx) = %d, expected %d\n", c, ret, expected);
    }
}

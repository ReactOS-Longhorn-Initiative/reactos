/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _itoa
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef char* (__cdecl *PFN__itoa)(int, char*, int);

START_TEST(_itoa)
{
    PFN__itoa p_itoa = (PFN__itoa)load_function("_itoa");
    char buffer[128];
    char* ret;
    size_t i;

    for (i = 0; i < g_xtoa_table_length; i++)
    {
        const xtoa_entry* e = &g_xtoa_table[i];

        if (!(e->flags & XF_I32))
            continue;

        if (e->radix < 2)
            continue;

        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_itoa((int)(__int64)e->value, buffer, e->radix);

        ok(ret == buffer,
            "[%d] _itoa(%d, %d) returned %p, expected %p\n",
            e->line, (int)(__int64)e->value, e->radix, ret, buffer);
        ok(strcmp(buffer, e->expected) == 0,
            "[%d] _itoa(%d, %d) = \"%s\", expected \"%s\"\n",
            e->line, (int)(__int64)e->value, e->radix, buffer, e->expected);
    }
}

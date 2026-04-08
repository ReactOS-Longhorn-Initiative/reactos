/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _i64toa
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef char* (__cdecl *PFN__i64toa)(__int64, char*, int);

START_TEST(_i64toa)
{
    PFN__i64toa p_i64toa = (PFN__i64toa)load_function("_i64toa");
    char buffer[128];
    char* ret;
    size_t i;

    for (i = 0; i < g_xtoa_table_length; i++)
    {
        const xtoa_entry* e = &g_xtoa_table[i];

        if (!(e->flags & XF_I64))
            continue;

        /* A radix < 2 crashes on Windows */
        if (e->radix < 2)
            continue;

        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_i64toa((__int64)e->value, buffer, e->radix);

        ok(ret == buffer,
            "[%d] _i64toa(%I64d, %d) returned %p, expected %p\n",
            e->line, (__int64)e->value, e->radix, ret, buffer);
        ok(strcmp(buffer, e->expected) == 0,
            "[%d] _i64toa(%I64d, %d) = \"%s\", expected \"%s\"\n",
            e->line, (__int64)e->value, e->radix, buffer, e->expected);
    }
}

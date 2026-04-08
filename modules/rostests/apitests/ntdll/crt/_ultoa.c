/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _ultoa
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef char* (__cdecl *PFN__ultoa)(unsigned long, char*, int);

START_TEST(_ultoa)
{
    PFN__ultoa p_ultoa = (PFN__ultoa)load_function("_ultoa");
    char buffer[128];
    char* ret;
    size_t i;

    for (i = 0; i < g_xtoa_table_length; i++)
    {
        const xtoa_entry* e = &g_xtoa_table[i];

        if (!(e->flags & XF_U32))
            continue;

        /* A radix < 2 crashes on Windows */
        if (e->radix < 2)
            continue;

        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_ultoa((unsigned long)e->value, buffer, e->radix);

        ok(ret == buffer,
            "[%d] _ultoa(%lu, %d) returned %p, expected %p\n",
            e->line, (unsigned long)e->value, e->radix, ret, buffer);
        ok(strcmp(buffer, e->expected) == 0,
            "[%d] _ultoa(%lu, %d) = \"%s\", expected \"%s\"\n",
            e->line, (unsigned long)e->value, e->radix, buffer, e->expected);
    }
}

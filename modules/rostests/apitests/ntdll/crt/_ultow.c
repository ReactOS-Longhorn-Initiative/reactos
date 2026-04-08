/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _ultow
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef wchar_t* (__cdecl *PFN__ultow)(unsigned long, wchar_t*, int);

START_TEST(_ultow)
{
    PFN__ultow p_ultow = (PFN__ultow)load_function("_ultow");
    wchar_t buffer[128];
    wchar_t* ret;
    size_t i;

    for (i = 0; i < g_xtow_table_length; i++)
    {
        const xtow_entry* e = &g_xtow_table[i];

        if (!(e->flags & XF_U32))
            continue;

        /* A radix < 2 crashes on Windows */
        if (e->radix < 2)
            continue;

        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_ultow((unsigned long)e->value, buffer, e->radix);

        ok(ret == buffer,
            "[%d] _ultow(%lu, %d) returned %p, expected %p\n",
            e->line, (unsigned long)e->value, e->radix, ret, buffer);
        ok(wcscmp(buffer, e->expected) == 0,
            "[%d] _ultow(%lu, %d) mismatch\n",
            e->line, (unsigned long)e->value, e->radix);
    }
}

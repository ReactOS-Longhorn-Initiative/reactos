/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _ui64tow
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef wchar_t* (__cdecl *PFN__ui64tow)(unsigned __int64, wchar_t*, int);

START_TEST(_ui64tow)
{
    PFN__ui64tow p_ui64tow = (PFN__ui64tow)load_function("_ui64tow");
    wchar_t buffer[128];
    wchar_t* ret;
    size_t i;

    for (i = 0; i < g_xtow_table_length; i++)
    {
        const xtow_entry* e = &g_xtow_table[i];

        if (!(e->flags & XF_U64))
            continue;

        if (e->radix < 2)
            continue;

        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_ui64tow(e->value, buffer, e->radix);

        ok(ret == buffer,
            "[%d] _ui64tow(%I64u, %d) returned %p, expected %p\n",
            e->line, e->value, e->radix, ret, buffer);
        ok(wcscmp(buffer, e->expected) == 0,
            "[%d] _ui64tow(%I64u, %d) mismatch\n",
            e->line, e->value, e->radix);
    }
}

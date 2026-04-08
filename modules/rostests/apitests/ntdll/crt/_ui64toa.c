/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _ui64toa
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef char* (__cdecl *PFN__ui64toa)(unsigned __int64, char*, int);

START_TEST(_ui64toa)
{
    PFN__ui64toa p_ui64toa = (PFN__ui64toa)load_function("_ui64toa");
    char buffer[128];
    char* ret;
    size_t i;

    for (i = 0; i < g_xtoa_table_length; i++)
    {
        const xtoa_entry* e = &g_xtoa_table[i];

        if (!(e->flags & XF_U64))
            continue;

        if (e->radix < 2)
            continue;

        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_ui64toa(e->value, buffer, e->radix);

        ok(ret == buffer,
            "[%d] _ui64toa(%I64u, %d) returned %p, expected %p\n",
            e->line, e->value, e->radix, ret, buffer);
        ok(strcmp(buffer, e->expected) == 0,
            "[%d] _ui64toa(%I64u, %d) = \"%s\", expected \"%s\"\n",
            e->line, e->value, e->radix, buffer, e->expected);
    }
}

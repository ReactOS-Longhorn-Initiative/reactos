/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for atoi
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef int (__cdecl *PFN_atoi)(const char*);

START_TEST(atoi)
{
    PFN_atoi p_atoi = (PFN_atoi)load_function("atoi");
    size_t i;

    for (i = 0; i < g_strtox_table_length; i++)
    {
        const strtox_entry* e = &g_strtox_table[i];
        int expected, result;

        /* atoi is decimal only */
        if (e->radix != 10)
            continue;

        expected = (int)(long)e->value;

        result = p_atoi(e->string);
        ok(result == expected,
            "[%d] atoi(\"%s\") = %d, expected %d\n",
            e->line, e->string, result, expected);
    }
}

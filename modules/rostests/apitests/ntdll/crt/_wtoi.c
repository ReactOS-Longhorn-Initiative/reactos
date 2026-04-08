/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wtoi
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef int (__cdecl *PFN__wtoi)(const wchar_t*);

START_TEST(_wtoi)
{
    PFN__wtoi p_wtoi = (PFN__wtoi)load_function("_wtoi");
    size_t i;

    for (i = 0; i < g_wcstox_table_length; i++)
    {
        const wcstox_entry* e = &g_wcstox_table[i];
        int expected, result;

        if (e->radix != 10)
            continue;

        expected = (int)(long)e->value;

        result = p_wtoi(e->string);
        ok(result == expected,
            "[%d] _wtoi(\"%S\") = %d, expected %d\n",
            e->line, e->string, result, expected);
    }
}

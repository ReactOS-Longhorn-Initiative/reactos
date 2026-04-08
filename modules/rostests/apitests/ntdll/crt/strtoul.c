/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strtoul
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef unsigned long (__cdecl *PFN_strtoul)(const char*, char**, int);

START_TEST(strtoul)
{
    PFN_strtoul p_strtoul = (PFN_strtoul)load_function("strtoul");
    size_t i;

    for (i = 0; i < g_strtox_table_length; i++)
    {
        const strtox_entry* e = &g_strtox_table[i];
        unsigned long expected, result;
        char *end = NULL;

        if ((e->flags & F_OVF_U32) && (e->flags & F_NEG))
            expected = (unsigned long)(0UL - ULONG_MAX);
        else if (e->flags & F_OVF_U32)
            expected = ULONG_MAX;
        else
            expected = (unsigned long)e->value;

        *ntdll_errno() = 0xdeadbeef;
        result = p_strtoul(e->string, &end, e->radix);

        ok(result == expected,
            "[%d] strtoul(\"%s\", %d) = %lu, expected %lu\n",
            e->line, e->string, e->radix, result, expected);
        ok(end == e->string + e->endptr_offset,
            "[%d] strtoul(\"%s\", %d) endptr offset = %d, expected %d\n",
            e->line, e->string, e->radix, (int)(end - e->string), e->endptr_offset);

        if (e->flags & F_OVF_U32)
            ok(*ntdll_errno() == ERANGE,
                "[%d] strtoul(\"%s\") errno = %d, expected ERANGE\n",
                e->line, e->string, *ntdll_errno());
        else
            ok(*ntdll_errno() == (int)0xdeadbeef,
                "[%d] strtoul(\"%s\") errno = 0x%x, expected 0xdeadbeef\n",
                e->line, e->string, *ntdll_errno());
    }

    /* NULL endptr */
    ok(p_strtoul("42", NULL, 10) == 42, "strtoul NULL endptr failed\n");
}

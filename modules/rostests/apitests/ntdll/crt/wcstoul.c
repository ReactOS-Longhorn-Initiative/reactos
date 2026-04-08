/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcstoul
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef unsigned long (__cdecl *PFN_wcstoul)(const wchar_t*, wchar_t**, int);

START_TEST(wcstoul)
{
    PFN_wcstoul p_wcstoul = (PFN_wcstoul)load_function("wcstoul");
    size_t i;

    for (i = 0; i < g_wcstox_table_length; i++)
    {
        const wcstox_entry* e = &g_wcstox_table[i];
        unsigned long expected, result;
        wchar_t *end = NULL;

        if ((e->flags & F_OVF_U32) && (e->flags & F_NEG))
            expected = (unsigned long)(0UL - ULONG_MAX);
        else if (e->flags & F_OVF_U32)
            expected = ULONG_MAX;
        else
            expected = (unsigned long)e->value;

        *ntdll_errno() = 0xdeadbeef;
        result = p_wcstoul(e->string, &end, e->radix);

        ok(result == expected,
            "[%d] wcstoul(\"%S\", %u) = %lu, expected %lu\n",
            e->line, e->string, e->radix, result, expected);
        ok(end == e->string + e->endptr_offset,
            "[%d] wcstoul(\"%S\", %u) endptr offset = %d, expected %d\n",
            e->line, e->string, e->radix, (int)(end - e->string), e->endptr_offset);

        if (e->flags & F_OVF_U32)
            ok(*ntdll_errno() == ERANGE,
                "[%d] wcstoul errno = %d, expected ERANGE\n",
                e->line, *ntdll_errno());
        else
            ok(*ntdll_errno() == (int)0xdeadbeef,
                "[%d] wcstoul errno = 0x%x, expected 0xdeadbeef\n",
                e->line, *ntdll_errno());
    }

    /* NULL endptr */
    ok(p_wcstoul(L"42", NULL, 10) == 42, "wcstoul NULL endptr failed\n");
}

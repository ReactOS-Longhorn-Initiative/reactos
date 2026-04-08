/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcstol
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef long (__cdecl *PFN_wcstol)(const wchar_t*, wchar_t**, int);

START_TEST(wcstol)
{
    PFN_wcstol p_wcstol = (PFN_wcstol)load_function("wcstol");
    size_t i;

    for (i = 0; i < g_wcstox_table_length; i++)
    {
        const wcstox_entry* e = &g_wcstox_table[i];
        long expected, result;
        wchar_t *end = NULL;

        if (e->flags & F_OVF_S32)
            expected = (e->flags & F_NEG) ? LONG_MIN : LONG_MAX;
        else
            expected = (long)e->value;

        *ntdll_errno() = 0xdeadbeef;
        result = p_wcstol(e->string, &end, e->radix);

        ok(result == expected,
            "[%d] wcstol(radix=%d) = %ld, expected %ld\n",
            e->line, e->radix, result, expected);
        ok(end == e->string + e->endptr_offset,
            "[%d] wcstol(radix=%d) endptr offset = %d, expected %d\n",
            e->line, e->radix, (int)(end - e->string), e->endptr_offset);

        if (e->flags & F_OVF_S32)
            ok(*ntdll_errno() == ERANGE,
                "[%d] wcstol errno = %d, expected ERANGE\n",
                e->line, *ntdll_errno());
        else
            ok(*ntdll_errno() == (int)0xdeadbeef,
                "[%d] wcstol errno = 0x%x, expected 0xdeadbeef\n",
                e->line, *ntdll_errno());
    }

    /* NULL endptr */
    ok(p_wcstol(L"42", NULL, 10) == 42, "wcstol NULL endptr failed\n");
}

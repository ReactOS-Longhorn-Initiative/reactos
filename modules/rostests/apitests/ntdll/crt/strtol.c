/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strtol
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef long (__cdecl *PFN_strtol)(const char*, char**, int);

START_TEST(strtol)
{
    PFN_strtol p_strtol = (PFN_strtol)load_function("strtol");
    size_t i;

    for (i = 0; i < g_strtox_table_length; i++)
    {
        const strtox_entry* e = &g_strtox_table[i];
        long expected, result;
        char *end = NULL;

        if (e->flags & F_OVF_S32)
            expected = (e->flags & F_NEG) ? LONG_MIN : LONG_MAX;
        else
            expected = (long)e->value;

        *ntdll_errno() = 0xdeadbeef;
        result = p_strtol(e->string, &end, e->radix);

        ok(result == expected,
            "[%d] strtol(\"%s\", %d) = %ld, expected %ld\n",
            e->line, e->string, e->radix, result, expected);
        ok(end == e->string + e->endptr_offset,
            "[%d] strtol(\"%s\", %d) endptr offset = %d, expected %d\n",
            e->line, e->string, e->radix, (int)(end - e->string), e->endptr_offset);

        if (e->flags & F_OVF_S32)
            ok(*ntdll_errno() == ERANGE,
                "[%d] strtol(\"%s\", %d) errno = %d, expected ERANGE\n",
                e->line, e->string, e->radix, *ntdll_errno());
        else
            ok(*ntdll_errno() == (int)0xdeadbeef,
                "[%d] strtol(\"%s\", %d) errno = 0x%x, expected 0xdeadbeef\n",
                e->line, e->string, e->radix, *ntdll_errno());
    }

    /* 0x prefix edge case: ntdll consumes 0x unconditionally,
       when no hex digit follows endptr = original string */
    {
        char *end;
        long ret;
        const char *s = "0x";

        ret = p_strtol(s, &end, 16);
        ok(ret == 0, "strtol(\"0x\", 16) = %ld\n", ret);
        ok(end == s, "strtol(\"0x\", 16) endptr offset = %d\n",
            (int)(end - s));
    }

    /* NULL endptr */
    ok(p_strtol("42", NULL, 10) == 42, "strtol NULL endptr failed\n");
}

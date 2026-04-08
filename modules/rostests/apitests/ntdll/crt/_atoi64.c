/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _atoi64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef __int64 (__cdecl *PFN__atoi64)(const char*);

START_TEST(_atoi64)
{
    PFN__atoi64 p_atoi64 = (PFN__atoi64)load_function("_atoi64");
    size_t i;

    for (i = 0; i < g_strtox_table_length; i++)
    {
        const strtox_entry* e = &g_strtox_table[i];
        __int64 expected, result;

        /* _atoi64 is decimal only; for non-decimal entries compute
           what base-10 parsing would yield */
        if (e->radix != 10)
            continue;

        if (e->flags & F_OVF_S64)
            expected = (e->flags & F_NEG) ? _I64_MIN : _I64_MAX;
        else
            expected = (__int64)e->value;

        *ntdll_errno() = 0xdeadbeef;
        result = p_atoi64(e->string);
        ok(result == expected,
            "[%d] _atoi64(\"%s\") = %I64d, expected %I64d\n",
            e->line, e->string, result, expected);

        if (e->flags & F_OVF_S64)
            ok(*ntdll_errno() == ERANGE,
                "[%d] _atoi64(\"%s\") errno = %d, expected ERANGE\n",
                e->line, e->string, *ntdll_errno());
        else
            ok(*ntdll_errno() == (int)0xdeadbeef,
                "[%d] _atoi64(\"%s\") errno = 0x%x, expected 0xdeadbeef\n",
                e->line, e->string, *ntdll_errno());
    }
}

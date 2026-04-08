/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wtoi64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef __int64 (__cdecl *PFN__wtoi64)(const wchar_t*);

START_TEST(_wtoi64)
{
    PFN__wtoi64 p_wtoi64 = (PFN__wtoi64)load_function("_wtoi64");
    size_t i;

    for (i = 0; i < g_wcstox_table_length; i++)
    {
        const wcstox_entry* e = &g_wcstox_table[i];
        __int64 expected, result;

        if (e->radix != 10)
            continue;

        if (e->flags & F_OVF_S64)
            expected = (e->flags & F_NEG) ? _I64_MIN : _I64_MAX;
        else
            expected = (__int64)e->value;

        *ntdll_errno() = 0xdeadbeef;
        result = p_wtoi64(e->string);
        ok(result == expected,
            "[%d] _wtoi64(\"%S\") = %I64d, expected %I64d\n",
            e->line, e->string, result, expected);

        if (e->flags & F_OVF_S64)
            ok(*ntdll_errno() == ERANGE,
                "[%d] _wtoi64 errno = %d, expected ERANGE\n",
                e->line, *ntdll_errno());
        else
            ok(*ntdll_errno() == (int)0xdeadbeef,
                "[%d] _wtoi64 errno = 0x%x, expected 0xdeadbeef\n",
                e->line, *ntdll_errno());
    }
}

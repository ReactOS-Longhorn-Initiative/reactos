/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wcstoui64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef unsigned __int64 (__cdecl *PFN__wcstoui64)(const wchar_t*, wchar_t**, int);

START_TEST(_wcstoui64)
{
    PFN__wcstoui64 p_wcstoui64 = (PFN__wcstoui64)load_function("_wcstoui64");
    size_t i;

    for (i = 0; i < g_wcstox_table_length; i++)
    {
        const wcstox_entry* e = &g_wcstox_table[i];
        unsigned __int64 expected, result;
        wchar_t *end = NULL;

        if (e->flags & F_OVF_U64)
            expected = _UI64_MAX;
        else
            expected = e->value;

        *ntdll_errno() = 0xdeadbeef;
        result = p_wcstoui64(e->string, &end, e->radix);

        ok(result == expected,
            "[%d] _wcstoui64(radix=%d) = %I64u, expected %I64u\n",
            e->line, e->radix, result, expected);
        ok(end == e->string + e->endptr_offset,
            "[%d] _wcstoui64(radix=%d) endptr offset = %d, expected %d\n",
            e->line, e->radix, (int)(end - e->string), e->endptr_offset);

        if (e->flags & F_OVF_U64)
            ok(*ntdll_errno() == ERANGE,
                "[%d] _wcstoui64 errno = %d, expected ERANGE\n",
                e->line, *ntdll_errno());
        else
            ok(*ntdll_errno() == (int)0xdeadbeef,
                "[%d] _wcstoui64 errno = 0x%x, expected 0xdeadbeef\n",
                e->line, *ntdll_errno());
    }

    /* NULL endptr */
    ok(p_wcstoui64(L"42", NULL, 10) == 42, "_wcstoui64 NULL endptr failed\n");
}

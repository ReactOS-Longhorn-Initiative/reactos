/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wcstoi64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "strtox_table.h"

typedef __int64 (__cdecl *PFN__wcstoi64)(const wchar_t*, wchar_t**, int);

START_TEST(_wcstoi64)
{
    PFN__wcstoi64 p_wcstoi64 = (PFN__wcstoi64)load_function("_wcstoi64");
    size_t i;

    if (p_wcstoi64 == NULL)
    {
        skip("Function _wcstoi64 not available\n");
        return;
    }

    for (i = 0; i < g_wcstox_table_length; i++)
    {
        const wcstox_entry* e = &g_wcstox_table[i];
        __int64 expected, result;
        wchar_t *end = NULL;

        if (e->flags & F_OVF_S64)
            expected = (e->flags & F_NEG) ? _I64_MIN : _I64_MAX;
        else
            expected = (__int64)e->value;

        *ntdll_errno() = 0xdeadbeef;
        result = p_wcstoi64(e->string, &end, e->radix);

        ok(result == expected,
            "[%d] _wcstoi64(radix=%d) = %I64d, expected %I64d\n",
            e->line, e->radix, result, expected);
        ok(end == e->string + e->endptr_offset,
            "[%d] _wcstoi64(radix=%d) endptr offset = %d, expected %d\n",
            e->line, e->radix, (int)(end - e->string), e->endptr_offset);

        if (e->flags & F_OVF_S64)
            ok(*ntdll_errno() == ERANGE,
                "[%d] _wcstoi64 errno = %d, expected ERANGE\n",
                e->line, *ntdll_errno());
        else
            ok(*ntdll_errno() == (int)0xdeadbeef,
                "[%d] _wcstoi64 errno = 0x%x, expected 0xdeadbeef\n",
                e->line, *ntdll_errno());
    }

    /* NULL endptr */
    ok(p_wcstoi64(L"42", NULL, 10) == 42, "_wcstoi64 NULL endptr failed\n");
}

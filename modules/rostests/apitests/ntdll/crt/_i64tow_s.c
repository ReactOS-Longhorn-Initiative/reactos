/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _i64tow_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef errno_t (__cdecl *PFN__i64tow_s)(__int64, wchar_t*, size_t, int);

START_TEST(_i64tow_s)
{
    PFN__i64tow_s p_i64tow_s = (PFN__i64tow_s)load_function("_i64tow_s");
    wchar_t buffer[128];
    errno_t ret;
    size_t i;

    if (!p_i64tow_s)
        return;

    *ntdll_errno() = 0xDEADBEEF;

    /* Table-driven tests */
    for (i = 0; i < g_xtow_table_length; i++)
    {
        const xtow_entry* e = &g_xtow_table[i];
        size_t expected_len;

        if (!(e->flags & XF_I64))
            continue;

        if ((e->radix < 2) || (e->radix > 36))
        {
            memset(buffer, 0xAA, sizeof(buffer));
            ret = p_i64tow_s((__int64)e->value, buffer, sizeof(buffer) / sizeof(wchar_t), e->radix);
            ok(ret == EINVAL,
                "[%d] _i64tow_s(%I64d, %d) returned %d, expected EINVAL\n",
                e->line, (__int64)e->value, e->radix, ret);
            ok(buffer[0] == L'\0',
                "[%d] _i64tow_s(%I64d, %d) buffer[0] = 0x%04x, expected 0\n",
                e->line, (__int64)e->value, e->radix, (unsigned)buffer[0]);
            continue;
        }

        expected_len = wcslen(e->expected);

        /* Test with sufficient buffer */
        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_i64tow_s((__int64)e->value, buffer, sizeof(buffer) / sizeof(wchar_t), e->radix);
        ok(ret == 0,
            "[%d] _i64tow_s(%I64d, %d) returned %d, expected 0\n",
            e->line, (__int64)e->value, e->radix, ret);
        ok(wcscmp(buffer, e->expected) == 0,
            "[%d] _i64tow_s(%I64d, %d) mismatch\n",
            e->line, (__int64)e->value, e->radix);

        /* Test with exact buffer size */
        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_i64tow_s((__int64)e->value, buffer, expected_len + 1, e->radix);
        ok(ret == 0,
            "[%d] _i64tow_s(%I64d, %d) exact size returned %d, expected 0\n",
            e->line, (__int64)e->value, e->radix, ret);
        ok(wcscmp(buffer, e->expected) == 0,
            "[%d] _i64tow_s(%I64d, %d) exact size mismatch\n",
            e->line, (__int64)e->value, e->radix);

        /* Test with buffer too small */
        if (expected_len > 0)
        {
            memset(buffer, 0xAA, sizeof(buffer));
            ret = p_i64tow_s((__int64)e->value, buffer, expected_len, e->radix);
            ok(ret == ERANGE,
                "[%d] _i64tow_s(%I64d, %d) small buf returned %d, expected ERANGE\n",
                e->line, (__int64)e->value, e->radix, ret);
            ok(buffer[0] == L'\0',
                "[%d] _i64tow_s(%I64d, %d) small buf: buffer[0] = 0x%04x, expected 0\n",
                e->line, (__int64)e->value, e->radix, (unsigned)buffer[0]);
        }
    }

    /* NULL buffer */
    ret = p_i64tow_s(42, NULL, 10, 10);
    ok(ret == EINVAL, "_i64tow_s(NULL) returned %d, expected EINVAL\n", ret);

    /* Zero buffer count */
    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_i64tow_s(42, buffer, 0, 10);
    ok(ret == EINVAL, "_i64tow_s(count=0) returned %d, expected EINVAL\n", ret);

    /* Invalid radix */
    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_i64tow_s(42, buffer, sizeof(buffer) / sizeof(wchar_t), 1);
    ok(ret == EINVAL, "_i64tow_s(radix=1) returned %d, expected EINVAL\n", ret);

    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_i64tow_s(42, buffer, sizeof(buffer) / sizeof(wchar_t), 37);
    ok(ret == EINVAL, "_i64tow_s(radix=37) returned %d, expected EINVAL\n", ret);

    ok_eq_errno(*ntdll_errno(), 0xDEADBEEF);
}

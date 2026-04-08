/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _i64toa_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef errno_t (__cdecl *PFN__i64toa_s)(__int64, char*, size_t, int);

START_TEST(_i64toa_s)
{
    PFN__i64toa_s p_i64toa_s = (PFN__i64toa_s)load_function("_i64toa_s");
    char buffer[128];
    errno_t ret;
    size_t i;

    if (!p_i64toa_s)
        return;

    *ntdll_errno() = 0xDEADBEEF;

    /* Table-driven tests */
    for (i = 0; i < g_xtoa_table_length; i++)
    {
        const xtoa_entry* e = &g_xtoa_table[i];
        size_t expected_len;

        if (!(e->flags & XF_I64))
            continue;

        if ((e->radix < 2) || (e->radix > 36))
        {
            memset(buffer, 0xAA, sizeof(buffer));
            ret = p_i64toa_s((__int64)e->value, buffer, sizeof(buffer), e->radix);
            ok(ret == EINVAL,
                "[%d] _i64toa_s(%I64d, %d) returned %d, expected EINVAL\n",
                e->line, (__int64)e->value, e->radix, ret);
            ok(buffer[0] == '\0',
                "[%d] _i64toa_s(%I64d, %d) buffer[0] = 0x%02x, expected 0\n",
                e->line, (__int64)e->value, e->radix, (unsigned char)buffer[0]);
            continue;
        }

        expected_len = strlen(e->expected);

        /* Test with sufficient buffer */
        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_i64toa_s((__int64)e->value, buffer, sizeof(buffer), e->radix);
        ok(ret == 0,
            "[%d] _i64toa_s(%I64d, %d) returned %d, expected 0\n",
            e->line, (__int64)e->value, e->radix, ret);
        ok(strcmp(buffer, e->expected) == 0,
            "[%d] _i64toa_s(%I64d, %d) = \"%s\", expected \"%s\"\n",
            e->line, (__int64)e->value, e->radix, buffer, e->expected);

        /* Test with exact buffer size */
        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_i64toa_s((__int64)e->value, buffer, expected_len + 1, e->radix);
        ok(ret == 0,
            "[%d] _i64toa_s(%I64d, %d) exact size returned %d, expected 0\n",
            e->line, (__int64)e->value, e->radix, ret);
        ok(strcmp(buffer, e->expected) == 0,
            "[%d] _i64toa_s(%I64d, %d) exact size = \"%s\", expected \"%s\"\n",
            e->line, (__int64)e->value, e->radix, buffer, e->expected);

        /* Test with buffer too small */
        if (expected_len > 0)
        {
            memset(buffer, 0xAA, sizeof(buffer));
            ret = p_i64toa_s((__int64)e->value, buffer, expected_len, e->radix);
            ok(ret == ERANGE,
                "[%d] _i64toa_s(%I64d, %d) small buf returned %d, expected ERANGE\n",
                e->line, (__int64)e->value, e->radix, ret);
            ok(buffer[0] == '\0',
                "[%d] _i64toa_s(%I64d, %d) small buf: buffer[0] = 0x%02x, expected 0\n",
                e->line, (__int64)e->value, e->radix, (unsigned char)buffer[0]);
        }
    }

    /* NULL buffer */
    ret = p_i64toa_s(42, NULL, 10, 10);
    ok(ret == EINVAL, "_i64toa_s(NULL) returned %d, expected EINVAL\n", ret);

    /* Zero buffer count */
    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_i64toa_s(42, buffer, 0, 10);
    ok(ret == EINVAL, "_i64toa_s(count=0) returned %d, expected EINVAL\n", ret);

    /* Invalid radix */
    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_i64toa_s(42, buffer, sizeof(buffer), 1);
    ok(ret == EINVAL, "_i64toa_s(radix=1) returned %d, expected EINVAL\n", ret);

    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_i64toa_s(42, buffer, sizeof(buffer), 37);
    ok(ret == EINVAL, "_i64toa_s(radix=37) returned %d, expected EINVAL\n", ret);

    ok_eq_errno(*ntdll_errno(), 0xDEADBEEF);
}

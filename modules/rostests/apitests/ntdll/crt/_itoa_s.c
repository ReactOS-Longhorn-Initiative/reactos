/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _itoa_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "xtostr_table.h"

typedef errno_t (__cdecl *PFN__itoa_s)(int, char*, size_t, int);

START_TEST(_itoa_s)
{
    PFN__itoa_s p_itoa_s = (PFN__itoa_s)load_function("_itoa_s");
    char buffer[128];
    errno_t ret;
    size_t i;

    if (!p_itoa_s)
        return;

    *ntdll_errno() = 0xDEADBEEF;

    /* Table-driven tests */
    for (i = 0; i < g_xtoa_table_length; i++)
    {
        const xtoa_entry* e = &g_xtoa_table[i];
        size_t expected_len;

        if (!(e->flags & XF_I32))
            continue;

        if ((e->radix < 2) || (e->radix > 36))
        {
            memset(buffer, 0xAA, sizeof(buffer));
            ret = p_itoa_s((int)(__int64)e->value, buffer, sizeof(buffer), e->radix);
            ok(ret == EINVAL,
                "[%d] _itoa_s(%d, %d) returned %d, expected EINVAL\n",
                e->line, (int)(__int64)e->value, e->radix, ret);
            ok(!memcmp(buffer, "\0\xAA", 2),
                "[%d] _itoa_s(%d, %d) buffer[0] = 0x%02x, expected 0\n",
                e->line, (int)(__int64)e->value, e->radix, (unsigned char)buffer[0]);
            continue;
        }

        expected_len = strlen(e->expected);

        /* Test with sufficient buffer */
        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_itoa_s((int)(__int64)e->value, buffer, sizeof(buffer), e->radix);
        ok(ret == 0,
            "[%d] _itoa_s(%d, %d) returned %d, expected 0\n",
            e->line, (int)(__int64)e->value, e->radix, ret);
        ok(strcmp(buffer, e->expected) == 0,
            "[%d] _itoa_s(%d, %d) = \"%s\", expected \"%s\"\n",
            e->line, (int)(__int64)e->value, e->radix, buffer, e->expected);

        /* Test with exact buffer size */
        memset(buffer, 0xAA, sizeof(buffer));
        ret = p_itoa_s((int)(__int64)e->value, buffer, expected_len + 1, e->radix);
        ok(ret == 0,
            "[%d] _itoa_s(%d, %d) exact size returned %d, expected 0\n",
            e->line, (int)(__int64)e->value, e->radix, ret);
        ok(strcmp(buffer, e->expected) == 0,
            "[%d] _itoa_s(%d, %d) exact size = \"%s\", expected \"%s\"\n",
            e->line, (int)(__int64)e->value, e->radix, buffer, e->expected);

        /* Test with buffer too small (only if result is non-empty) */
        if (expected_len > 0)
        {
            memset(buffer, 0xAA, sizeof(buffer));
            ret = p_itoa_s((int)(__int64)e->value, buffer, expected_len, e->radix);
            ok(ret == ERANGE,
                "[%d] _itoa_s(%d, %d) small buf returned %d, expected ERANGE\n",
                e->line, (int)(__int64)e->value, e->radix, ret);
            ok(buffer[0] == '\0',
                "[%d] _itoa_s(%d, %d) small buf: buffer[0] = 0x%02x, expected 0\n",
                e->line, (int)(__int64)e->value, e->radix, (unsigned char)buffer[0]);
        }
    }

    /* NULL buffer */
    ret = p_itoa_s(42, NULL, 10, 10);
    ok(ret == EINVAL, "_itoa_s(NULL) returned %d, expected EINVAL\n", ret);

    /* Zero buffer count */
    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_itoa_s(42, buffer, 0, 10);
    ok(ret == EINVAL, "_itoa_s(count=0) returned %d, expected EINVAL\n", ret);

    /* Invalid radix */
    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_itoa_s(42, buffer, sizeof(buffer), 1);
    ok(ret == EINVAL, "_itoa_s(radix=1) returned %d, expected EINVAL\n", ret);

    memset(buffer, 0xAA, sizeof(buffer));
    ret = p_itoa_s(42, buffer, sizeof(buffer), 37);
    ok(ret == EINVAL, "_itoa_s(radix=37) returned %d, expected EINVAL\n", ret);

    ok_eq_errno(*ntdll_errno(), 0xDEADBEEF);
}

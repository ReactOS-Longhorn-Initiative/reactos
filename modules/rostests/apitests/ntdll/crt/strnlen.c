/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strnlen
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef size_t (__cdecl *PFN_strnlen)(const char* _String, size_t _MaxCount);

START_TEST(strnlen)
{
    PFN_strnlen p_strnlen = (PFN_strnlen)load_function("strnlen");

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_strnlen, NULL);
        return;
    }

    if (p_strnlen == NULL)
    {
        ok(FALSE, "strnlen is not available\n");
        return;
    }

    /* Empty string with various max counts */
    ok_eq_size(p_strnlen("", 0), 0);
    ok_eq_size(p_strnlen("", 1), 0);
    ok_eq_size(p_strnlen("", 100), 0);

    /* String shorter than max count */
    ok_eq_size(p_strnlen("Hello", 10), 5);
    ok_eq_size(p_strnlen("Hello", 100), 5);

    /* String length equals max count */
    ok_eq_size(p_strnlen("Hello", 5), 5);

    /* Max count shorter than string - should return max count */
    ok_eq_size(p_strnlen("Hello", 3), 3);
    ok_eq_size(p_strnlen("Hello", 1), 1);
    ok_eq_size(p_strnlen("Hello", 0), 0);

    /* Max count of zero always returns 0 */
    ok_eq_size(p_strnlen("anything", 0), 0);

    /* String with embedded null - stops at first null */
    char buf[16] = "ABC\0DEF";
    ok_eq_size(p_strnlen(buf, 10), 3);
    ok_eq_size(p_strnlen(buf, 3), 3);
    ok_eq_size(p_strnlen(buf, 2), 2);

    /* Test various lengths from 0 to 64 with exact max count */
    char buf2[65];
    for (size_t i = 0; i <= 64; i++)
    {
        memset(buf2, 'x', i);
        buf2[i] = '\0';

        /* Max count larger than string */
        ok(p_strnlen(buf2, 65) == i,
            "strnlen(len=%zu, max=65) = %zu, expected %zu\n",
            i, p_strnlen(buf2, 65), i);

        /* Max count equal to string length */
        ok(p_strnlen(buf2, i) == i,
            "strnlen(len=%zu, max=%zu) = %zu, expected %zu\n",
            i, i, p_strnlen(buf2, i), i);

        /* Max count one less than string length (except for empty) */
        if (i > 0)
        {
            ok(p_strnlen(buf2, i - 1) == i - 1,
                "strnlen(len=%zu, max=%zu) = %zu, expected %zu\n",
                i, i - 1, p_strnlen(buf2, i - 1), i - 1);
        }
    }
}

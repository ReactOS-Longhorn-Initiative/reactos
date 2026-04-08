/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strlen
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef size_t (__cdecl *PFN_strlen)(const char* _Str);

START_TEST(strlen)
{
    PFN_strlen p_strlen = (PFN_strlen)load_function("strlen");

    /* Empty string */
    ok_eq_size(p_strlen(""), 0);

    /* Single character */
    ok_eq_size(p_strlen("A"), 1);

    /* Typical string */
    ok_eq_size(p_strlen("Hello"), 5);

    /* String with special characters */
    ok_eq_size(p_strlen("Hello, World! 123 \t\n"), 20);

    /* String with high bytes */
    ok_eq_size(p_strlen("\x80\xAB\xFE"), 3);

    /* Longer string */
    ok_eq_size(p_strlen("abcdefghijklmnopqrstuvwxyz"), 26);

    /* String with embedded content after null */
    char buf[16] = "ABC\0DEF";
    ok_eq_size(p_strlen(buf), 3);

    /* Test various lengths from 0 to 64 */
    char buf2[65];
    for (size_t i = 0; i <= 64; i++)
    {
        memset(buf2, 'x', i);
        buf2[i] = '\0';
        ok(p_strlen(buf2) == i,
            "strlen(len=%zu) = %zu, expected %u\n",
            i, p_strlen(buf2), i);
    }
}

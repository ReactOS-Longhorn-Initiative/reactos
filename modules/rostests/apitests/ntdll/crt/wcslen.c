/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcslen
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef size_t (__cdecl *PFN_wcslen)(const wchar_t* _String);

START_TEST(wcslen)
{
    PFN_wcslen p_wcslen = (PFN_wcslen)load_function("wcslen");

    /* Empty string */
    ok_eq_size(p_wcslen(L""), 0);

    /* Single character */
    ok_eq_size(p_wcslen(L"A"), 1);

    /* Typical string */
    ok_eq_size(p_wcslen(L"Hello"), 5);

    /* String with special characters */
    ok_eq_size(p_wcslen(L"Hello, World! 123 \t\n"), 20);

    /* String with high Unicode values */
    ok_eq_size(p_wcslen(L"\x0080\x00AB\x00FE\xFF00\xFFFE"), 5);

    /* Longer string */
    ok_eq_size(p_wcslen(L"abcdefghijklmnopqrstuvwxyz"), 26);

    /* String with embedded content after null */
    wchar_t buf[16] = L"ABC\0DEF";
    ok_eq_size(p_wcslen(buf), 3);

    /* Test various lengths from 0 to 64 */
    wchar_t buf2[65];
    for (size_t i = 0; i <= 64; i++)
    {
        for (size_t j = 0; j < i; j++)
            buf2[j] = L'x';
        buf2[i] = L'\0';
        ok(p_wcslen(buf2) == i,
           "wcslen(len=%u) = %zu, expected %zu\n",
           i, p_wcslen(buf2), i);
    }
}

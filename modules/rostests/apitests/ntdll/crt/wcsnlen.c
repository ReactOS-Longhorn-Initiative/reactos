/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcsnlen
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef size_t (__cdecl *PFN_wcsnlen)(const wchar_t* _Source, size_t _MaxCount);

START_TEST(wcsnlen)
{
    PFN_wcsnlen p_wcsnlen = (PFN_wcsnlen)load_function("wcsnlen");

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_wcsnlen, NULL);
        return;
    }

    if (p_wcsnlen == NULL)
    {
        ok(FALSE, "wcsnlen is not available\n");
        return;
    }

    /* Empty string with various max counts */
    ok_eq_size(p_wcsnlen(L"", 0), 0);
    ok_eq_size(p_wcsnlen(L"", 1), 0);
    ok_eq_size(p_wcsnlen(L"", 100), 0);

    /* String shorter than max count */
    ok_eq_size(p_wcsnlen(L"Hello", 10), 5);
    ok_eq_size(p_wcsnlen(L"Hello", 100), 5);

    /* String length equals max count */
    ok_eq_size(p_wcsnlen(L"Hello", 5), 5);

    /* Max count shorter than string - should return max count */
    ok_eq_size(p_wcsnlen(L"Hello", 3), 3);
    ok_eq_size(p_wcsnlen(L"Hello", 1), 1);
    ok_eq_size(p_wcsnlen(L"Hello", 0), 0);

    /* Max count of zero always returns 0 */
    ok_eq_size(p_wcsnlen(L"anything", 0), 0);

    /* String with embedded null - stops at first null */
    wchar_t buf[16] = L"ABC\0DEF";
    ok_eq_size(p_wcsnlen(buf, 10), 3);
    ok_eq_size(p_wcsnlen(buf, 3), 3);
    ok_eq_size(p_wcsnlen(buf, 2), 2);

    /* String with high Unicode values */
    ok_eq_size(p_wcsnlen(L"\xFF00\xFFFE\x0100", 10), 3);

    /* Test various lengths from 0 to 64 with exact max count */
    wchar_t buf2[65];
    for (size_t i = 0; i <= 64; i++)
    {
        for (size_t j = 0; j < i; j++)
            buf2[j] = L'x';
        buf2[i] = L'\0';

        /* Max count larger than string */
        ok(p_wcsnlen(buf2, 65) == i,
           "wcsnlen(len=%zu, max=65) = %zu, expected %zu\n",
           i, p_wcsnlen(buf2, 65), i);

        /* Max count equal to string length */
        ok(p_wcsnlen(buf2, i) == i,
           "wcsnlen(len=%zu, max=%zu) = %zu, expected %zu\n",
           i, i, p_wcsnlen(buf2, i), i);

        /* Max count one less than string length (except for empty) */
        if (i > 0)
        {
            ok(p_wcsnlen(buf2, i - 1) == i - 1,
               "wcsnlen(len=%zu, max=%zu) = %zu, expected %zu\n",
               i, i - 1, p_wcsnlen(buf2, i - 1), i - 1);
        }
    }
}

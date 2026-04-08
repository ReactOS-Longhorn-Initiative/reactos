/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcsncat
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef wchar_t* (__cdecl *PFN_wcsncat)(wchar_t* _Destination, const wchar_t* _Source, size_t _Count);

START_TEST(wcsncat)
{
    PFN_wcsncat p_wcsncat = (PFN_wcsncat)load_function("wcsncat");
    wchar_t dst[64];
    wchar_t *ret;

    /* Basic: count >= source length */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat(dst, L" World", 10);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"Hello World") == 0, "wcsncat basic result mismatch\n");
    ok(dst[12] == (wchar_t)0xAAAA, "wcsncat wrote past null\n");

    /* Count limits the copy */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat(dst, L" World", 3);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"Hello Wo") == 0, "wcsncat limited result mismatch\n");
    ok(dst[9] == (wchar_t)0xAAAA, "wcsncat limited wrote past null\n");

    /* Count = 0 appends nothing */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat(dst, L" World", 0);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"Hello") == 0, "wcsncat zero-count result mismatch\n");
    ok(dst[6] == (wchar_t)0xAAAA, "wcsncat zero-count wrote past null\n");

    /* Append to empty string */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = L'\0';
    ret = p_wcsncat(dst, L"Test", 10);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"Test") == 0, "wcsncat to-empty result mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcsncat to-empty wrote past null\n");

    /* Append empty string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat(dst, L"", 10);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"Hello") == 0, "wcsncat empty-append result mismatch\n");
    ok(dst[6] == (wchar_t)0xAAAA, "wcsncat empty-append wrote past null\n");

    /* Source shorter than count */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"AB", 3 * sizeof(wchar_t));
    ret = p_wcsncat(dst, L"CD", 10);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"ABCD") == 0, "wcsncat short-src result mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcsncat short-src wrote past null\n");

    /* Count exactly matches source length */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcsncat(dst, L"XYZ", 3);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"HiXYZ") == 0, "wcsncat exact-count result mismatch\n");
    ok(dst[6] == (wchar_t)0xAAAA, "wcsncat exact-count wrote past null\n");

    /* High Unicode values */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"\xFF00", 2 * sizeof(wchar_t));
    ret = p_wcsncat(dst, L"\xFFFE\x0100", 2);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok((dst[0] == (wchar_t)0xFF00) && (dst[1] == (wchar_t)0xFFFE) &&
       (dst[2] == (wchar_t)0x0100) && (dst[3] == L'\0') &&
       (dst[4] == (wchar_t)0xAAAA),
       "wcsncat unicode content mismatch\n");

    /* Both empty with count 0 */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = L'\0';
    ret = p_wcsncat(dst, L"", 0);
    ok(ret == dst, "wcsncat returned %p, expected %p\n", ret, dst);
    ok((dst[0] == L'\0') && (dst[1] == (wchar_t)0xAAAA),
       "wcsncat both-empty content mismatch\n");
}

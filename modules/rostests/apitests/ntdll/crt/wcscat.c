/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcscat
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef wchar_t* (__cdecl *PFN_wcscat)(wchar_t* _Destination, const wchar_t* _Source);

START_TEST(wcscat)
{
    PFN_wcscat p_wcscat = (PFN_wcscat)load_function("wcscat");
    wchar_t dst[64];
    wchar_t *ret;

    /* Basic concatenation */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcscat(dst, L" World");
    ok(ret == dst, "wcscat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"Hello World") == 0, "wcscat basic result mismatch\n");
    ok(dst[11] == L'\0', "wcscat null terminator missing\n");
    ok(dst[12] == (wchar_t)0xAAAA, "wcscat wrote past null terminator\n");

    /* Append to empty string */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = L'\0';
    ret = p_wcscat(dst, L"Test");
    ok(ret == dst, "wcscat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"Test") == 0, "wcscat to-empty result mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcscat to-empty wrote past null\n");

    /* Append empty string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcscat(dst, L"");
    ok(ret == dst, "wcscat returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"Hello") == 0, "wcscat empty-append result mismatch\n");
    ok(dst[6] == (wchar_t)0xAAAA, "wcscat empty-append wrote past null\n");

    /* Both empty */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = L'\0';
    ret = p_wcscat(dst, L"");
    ok(ret == dst, "wcscat returned %p, expected %p\n", ret, dst);
    ok(dst[0] == L'\0', "wcscat both-empty: dst[0] != 0\n");
    ok(dst[1] == (wchar_t)0xAAAA, "wcscat both-empty wrote past null\n");

    /* High Unicode values */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"\xFF00", 2 * sizeof(wchar_t));
    ret = p_wcscat(dst, L"\xFFFE");
    ok(ret == dst, "wcscat returned %p, expected %p\n", ret, dst);
    ok((dst[0] == (wchar_t)0xFF00) && (dst[1] == (wchar_t)0xFFFE) &&
       (dst[2] == L'\0') && (dst[3] == (wchar_t)0xAAAA),
       "wcscat high unicode content mismatch\n");

    /* Multiple concatenations */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = L'\0';
    p_wcscat(dst, L"abc");
    p_wcscat(dst, L"def");
    p_wcscat(dst, L"ghi");
    ok(wcscmp(dst, L"abcdefghi") == 0, "wcscat multi result mismatch\n");
    ok(dst[10] == (wchar_t)0xAAAA, "wcscat multi wrote past null\n");
}

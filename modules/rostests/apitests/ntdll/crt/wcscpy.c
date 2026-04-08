/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcscpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef wchar_t* (__cdecl *PFN_wcscpy)(wchar_t* _Destination, const wchar_t* _Source);

START_TEST(wcscpy)
{
    PFN_wcscpy p_wcscpy = (PFN_wcscpy)load_function("wcscpy");
    wchar_t dst[64];
    wchar_t *ret;

    /* Basic string copy */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy(dst, L"Hello");
    ok(ret == dst, "wcscpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, L"Hello", 5 * sizeof(wchar_t)) == 0, "wcscpy basic content mismatch\n");
    ok(dst[5] == L'\0', "wcscpy null terminator missing\n");
    ok(dst[6] == (wchar_t)0xAAAA, "wcscpy wrote past null terminator\n");

    /* Single character */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy(dst, L"A");
    ok(ret == dst, "wcscpy returned %p, expected %p\n", ret, dst);
    ok((dst[0] == L'A') && (dst[1] == L'\0') && (dst[2] == (wchar_t)0xAAAA),
       "wcscpy single char content mismatch\n");

    /* Empty string */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy(dst, L"");
    ok(ret == dst, "wcscpy returned %p, expected %p\n", ret, dst);
    ok((dst[0] == L'\0') && (dst[1] == (wchar_t)0xAAAA),
       "wcscpy empty content mismatch\n");

    /* String with high Unicode values */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy(dst, L"\xFF00\xFFFE");
    ok(ret == dst, "wcscpy returned %p, expected %p\n", ret, dst);
    ok((dst[0] == (wchar_t)0xFF00) && (dst[1] == (wchar_t)0xFFFE) &&
       (dst[2] == L'\0') && (dst[3] == (wchar_t)0xAAAA),
       "wcscpy high unicode content mismatch\n");

    /* Overwrite existing content */
    memset(dst, 0xAA, sizeof(dst));
    p_wcscpy(dst, L"LongString12345");
    p_wcscpy(dst, L"Short");
    ok(wcscmp(dst, L"Short") == 0, "wcscpy overwrite result mismatch\n");
    ok((dst[6] == L'r') && (dst[7] == L'i'), "wcscpy old data not preserved\n");

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcscpy(dst, dst);
    ok(ret == dst, "wcscpy self returned %p, expected %p\n", ret, dst);
    ok((wcscmp(dst, L"Hello") == 0) && (dst[6] == (wchar_t)0xAAAA),
       "wcscpy self content mismatch\n");

    /* Overlapping: src at positive offset from dst */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello World\0", 12 * sizeof(wchar_t));
    ret = p_wcscpy(dst, dst + 6);
    ok(ret == dst, "wcscpy fwd-overlap returned %p, expected %p\n", ret, dst);
    ok(wcscmp(dst, L"World") == 0, "wcscpy fwd-overlap result mismatch\n");
    ok(memcmp(dst + 6, L"World\0", 6 * sizeof(wchar_t)) == 0, "wcscpy fwd-overlap old data changed\n");
}

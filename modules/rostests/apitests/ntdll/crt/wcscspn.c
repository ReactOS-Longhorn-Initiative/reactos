/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcscspn
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef size_t (__cdecl *PFN_wcscspn)(const wchar_t* _Str, const wchar_t* _Control);

START_TEST(wcscspn)
{
    PFN_wcscspn p_wcscspn = (PFN_wcscspn)load_function("wcscspn");
    size_t ret;

    /* Basic: first rejection char */
    ret = p_wcscspn(L"abcdef", L"de");
    ok(ret == 3, "wcscspn(\"abcdef\", \"de\") = %zu, expected 3\n", ret);

    /* No rejection chars found - returns string length */
    ret = p_wcscspn(L"abcdef", L"xyz");
    ok(ret == 6, "wcscspn(\"abcdef\", \"xyz\") = %zu, expected 6\n", ret);

    /* First char is rejected */
    ret = p_wcscspn(L"abcdef", L"a");
    ok(ret == 0, "wcscspn(\"abcdef\", \"a\") = %zu, expected 0\n", ret);

    /* Last char is rejected */
    ret = p_wcscspn(L"abcdef", L"f");
    ok(ret == 5, "wcscspn(\"abcdef\", \"f\") = %zu, expected 5\n", ret);

    /* Empty string */
    ret = p_wcscspn(L"", L"abc");
    ok(ret == 0, "wcscspn(\"\", \"abc\") = %zu, expected 0\n", ret);

    /* Empty control set - returns string length */
    ret = p_wcscspn(L"abc", L"");
    ok(ret == 3, "wcscspn(\"abc\", \"\") = %zu, expected 3\n", ret);

    /* Both empty */
    ret = p_wcscspn(L"", L"");
    ok(ret == 0, "wcscspn(\"\", \"\") = %zu, expected 0\n", ret);

    /* Single char string, not in control */
    ret = p_wcscspn(L"x", L"abc");
    ok(ret == 1, "wcscspn(\"x\", \"abc\") = %zu, expected 1\n", ret);

    /* Single char string, in control */
    ret = p_wcscspn(L"a", L"abc");
    ok(ret == 0, "wcscspn(\"a\", \"abc\") = %zu, expected 0\n", ret);

    /* Case sensitive */
    ret = p_wcscspn(L"ABCdef", L"abc");
    ok(ret == 6, "wcscspn case = %zu, expected 6\n", ret);

    /* High Unicode values in control */
    ret = p_wcscspn(L"abc\xFF00" L"def", L"\xFF00\xFFFE");
    ok(ret == 3, "wcscspn high unicode = %zu, expected 3\n", ret);

    /* High Unicode values not in control */
    ret = p_wcscspn(L"\xFF00\xFFFE\x0100", L"abc");
    ok(ret == 3, "wcscspn high unicode no match = %zu, expected 3\n", ret);

    /* Space separator */
    ret = p_wcscspn(L"hello world!", L" ");
    ok(ret == 5, "wcscspn space = %zu, expected 5\n", ret);
}

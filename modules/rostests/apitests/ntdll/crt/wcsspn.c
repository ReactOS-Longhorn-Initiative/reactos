/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcsspn
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef size_t (__cdecl *PFN_wcsspn)(const wchar_t* _Str, const wchar_t* _Control);

START_TEST(wcsspn)
{
    PFN_wcsspn p_wcsspn = (PFN_wcsspn)load_function("wcsspn");
    size_t ret;

    /* Basic: initial segment matches */
    ret = p_wcsspn(L"abcdef", L"abc");
    ok(ret == 3, "wcsspn(\"abcdef\", \"abc\") = %zu, expected 3\n", ret);

    /* Full string matches */
    ret = p_wcsspn(L"aabbcc", L"abc");
    ok(ret == 6, "wcsspn(\"aabbcc\", \"abc\") = %zu, expected 6\n", ret);

    /* No match at start */
    ret = p_wcsspn(L"xyzabc", L"abc");
    ok(ret == 0, "wcsspn(\"xyzabc\", \"abc\") = %zu, expected 0\n", ret);

    /* Empty string */
    ret = p_wcsspn(L"", L"abc");
    ok(ret == 0, "wcsspn(\"\", \"abc\") = %zu, expected 0\n", ret);

    /* Empty control set */
    ret = p_wcsspn(L"abc", L"");
    ok(ret == 0, "wcsspn(\"abc\", \"\") = %zu, expected 0\n", ret);

    /* Both empty */
    ret = p_wcsspn(L"", L"");
    ok(ret == 0, "wcsspn(\"\", \"\") = %zu, expected 0\n", ret);

    /* Single char match */
    ret = p_wcsspn(L"aaab", L"a");
    ok(ret == 3, "wcsspn(\"aaab\", \"a\") = %zu, expected 3\n", ret);

    /* Single char no match */
    ret = p_wcsspn(L"baaa", L"a");
    ok(ret == 0, "wcsspn(\"baaa\", \"a\") = %zu, expected 0\n", ret);

    /* Case sensitive */
    ret = p_wcsspn(L"aAbBcC", L"abc");
    ok(ret == 1, "wcsspn case sensitive = %zu, expected 1\n", ret);

    /* High Unicode values in control */
    ret = p_wcsspn(L"\xFF00\xFFFE\x0100" L"abc", L"\xFF00\xFFFE\x0100");
    ok(ret == 3, "wcsspn high unicode = %zu, expected 3\n", ret);

    /* High Unicode value stops match */
    ret = p_wcsspn(L"abc\xFF00" L"def", L"abcdef");
    ok(ret == 3, "wcsspn high unicode break = %zu, expected 3\n", ret);

    /* Digits */
    ret = p_wcsspn(L"123-456", L"0123456789");
    ok(ret == 3, "wcsspn digits = %zu, expected 3\n", ret);
}

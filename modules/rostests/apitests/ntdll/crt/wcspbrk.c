/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcspbrk
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef wchar_t* (__cdecl *PFN_wcspbrk)(const wchar_t* _Str, const wchar_t* _Control);

START_TEST(wcspbrk)
{
    PFN_wcspbrk p_wcspbrk = (PFN_wcspbrk)load_function("wcspbrk");
    const wchar_t *str;
    wchar_t *ret;

    /* Basic: find first matching char */
    str = L"abcdef";
    ret = p_wcspbrk(str, L"dc");
    ok(ret == str + 2, "wcspbrk(\"abcdef\", \"dc\") = %p, expected %p\n", ret, str + 2);

    /* Match at first char */
    str = L"abcdef";
    ret = p_wcspbrk(str, L"a");
    ok(ret == str, "wcspbrk(\"abcdef\", \"a\") = %p, expected %p\n", ret, str);

    /* Match at last char */
    str = L"abcdef";
    ret = p_wcspbrk(str, L"f");
    ok(ret == str + 5, "wcspbrk(\"abcdef\", \"f\") = %p, expected %p\n", ret, str + 5);

    /* No match - returns NULL */
    ret = p_wcspbrk(L"abcdef", L"xyz");
    ok(ret == NULL, "wcspbrk(\"abcdef\", \"xyz\") = %p, expected NULL\n", ret);

    /* Empty string - returns NULL */
    ret = p_wcspbrk(L"", L"abc");
    ok(ret == NULL, "wcspbrk(\"\", \"abc\") = %p, expected NULL\n", ret);

    /* Empty control - returns NULL */
    ret = p_wcspbrk(L"abc", L"");
    ok(ret == NULL, "wcspbrk(\"abc\", \"\") = %p, expected NULL\n", ret);

    /* Both empty - returns NULL */
    ret = p_wcspbrk(L"", L"");
    ok(ret == NULL, "wcspbrk(\"\", \"\") = %p, expected NULL\n", ret);

    /* Multiple matches - returns first */
    str = L"abcabc";
    ret = p_wcspbrk(str, L"cb");
    ok(ret == str + 1, "wcspbrk(\"abcabc\", \"cb\") = %p, expected %p\n", ret, str + 1);

    /* Case sensitive */
    ret = p_wcspbrk(L"ABCDEF", L"abc");
    ok(ret == NULL, "wcspbrk case = %p, expected NULL\n", ret);

    /* High Unicode values */
    str = L"abc\xFF00\xFFFE";
    ret = p_wcspbrk(str, L"\xFFFE\x0100");
    ok(ret == str + 4, "wcspbrk high unicode = %p, expected %p\n", ret, str + 4);

    /* High Unicode not in control */
    str = L"\xFF00\xFFFE\x0100";
    ret = p_wcspbrk(str, L"abc");
    ok(ret == NULL, "wcspbrk high no match = %p, expected NULL\n", ret);

    /* Single char string, found */
    str = L"a";
    ret = p_wcspbrk(str, L"a");
    ok(ret == str, "wcspbrk single match = %p, expected %p\n", ret, str);

    /* Single char string, not found */
    ret = p_wcspbrk(L"x", L"abc");
    ok(ret == NULL, "wcspbrk single no match = %p, expected NULL\n", ret);

    /* Space as separator */
    str = L"hello world";
    ret = p_wcspbrk(str, L" \t\n");
    ok(ret == str + 5, "wcspbrk space = %p, expected %p\n", ret, str + 5);
}

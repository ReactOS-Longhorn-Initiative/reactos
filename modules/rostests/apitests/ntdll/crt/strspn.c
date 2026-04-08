/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strspn
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef size_t (__cdecl *PFN_strspn)(const char* _Str, const char* _Control);

START_TEST(strspn)
{
    PFN_strspn p_strspn = (PFN_strspn)load_function("strspn");
    size_t ret;

    /* Basic: initial segment matches */
    ret = p_strspn("abcdef", "abc");
    ok(ret == 3, "strspn(\"abcdef\", \"abc\") = %zu, expected 3\n", ret);

    /* Full string matches */
    ret = p_strspn("aabbcc", "abc");
    ok(ret == 6, "strspn(\"aabbcc\", \"abc\") = %zu, expected 6\n", ret);

    /* No match at start */
    ret = p_strspn("xyzabc", "abc");
    ok(ret == 0, "strspn(\"xyzabc\", \"abc\") = %zu, expected 0\n", ret);

    /* Empty string */
    ret = p_strspn("", "abc");
    ok(ret == 0, "strspn(\"\", \"abc\") = %zu, expected 0\n", ret);

    /* Empty control set */
    ret = p_strspn("abc", "");
    ok(ret == 0, "strspn(\"abc\", \"\") = %zu, expected 0\n", ret);

    /* Both empty */
    ret = p_strspn("", "");
    ok(ret == 0, "strspn(\"\", \"\") = %zu, expected 0\n", ret);

    /* Single char match */
    ret = p_strspn("aaab", "a");
    ok(ret == 3, "strspn(\"aaab\", \"a\") = %zu, expected 3\n", ret);

    /* Single char no match */
    ret = p_strspn("baaa", "a");
    ok(ret == 0, "strspn(\"baaa\", \"a\") = %zu, expected 0\n", ret);

    /* Control has duplicates */
    ret = p_strspn("abcdef", "aabbcc");
    ok(ret == 3, "strspn with dup control = %zu, expected 3\n", ret);

    /* Partial match in middle */
    ret = p_strspn("aAbBcC", "abc");
    ok(ret == 1, "strspn case sensitive = %zu, expected 1\n", ret);

    /* High byte values in control */
    ret = p_strspn("\x80\xFE\xFF" "abc", "\x80\xFE\xFF");
    ok(ret == 3, "strspn high bytes = %zu, expected 3\n", ret);

    /* High byte value stops match */
    ret = p_strspn("abc\x80" "def", "abcdef");
    ok(ret == 3, "strspn high byte break = %zu, expected 3\n", ret);

    /* Digits and special chars */
    ret = p_strspn("123-456", "0123456789");
    ok(ret == 3, "strspn digits = %zu, expected 3\n", ret);

    /* All 256 byte values in control - match everything until null */
    {
        char control[256];
        for (int i = 0; i < 255; i++)
            control[i] = (char)(i + 1);
        control[255] = '\0';
        ret = p_strspn("Hello World!", control);
        ok(ret == 12, "strspn all-chars = %zu, expected 12\n", ret);
    }
}

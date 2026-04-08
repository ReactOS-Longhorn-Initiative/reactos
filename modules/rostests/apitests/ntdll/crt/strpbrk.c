/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strpbrk
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef char* (__cdecl *PFN_strpbrk)(const char* _Str, const char* _Control);

START_TEST(strpbrk)
{
    PFN_strpbrk p_strpbrk = (PFN_strpbrk)load_function("strpbrk");
    const char *str;
    char *ret;

    /* Basic: find first matching char */
    str = "abcdef";
    ret = p_strpbrk(str, "dc");
    ok(ret == str + 2, "strpbrk(\"abcdef\", \"dc\") = %p, expected %p\n", ret, str + 2);

    /* Match at first char */
    str = "abcdef";
    ret = p_strpbrk(str, "a");
    ok(ret == str, "strpbrk(\"abcdef\", \"a\") = %p, expected %p\n", ret, str);

    /* Match at last char */
    str = "abcdef";
    ret = p_strpbrk(str, "f");
    ok(ret == str + 5, "strpbrk(\"abcdef\", \"f\") = %p, expected %p\n", ret, str + 5);

    /* No match - returns NULL */
    ret = p_strpbrk("abcdef", "xyz");
    ok(ret == NULL, "strpbrk(\"abcdef\", \"xyz\") = %p, expected NULL\n", ret);

    /* Empty string - returns NULL */
    ret = p_strpbrk("", "abc");
    ok(ret == NULL, "strpbrk(\"\", \"abc\") = %p, expected NULL\n", ret);

    /* Empty control - returns NULL */
    ret = p_strpbrk("abc", "");
    ok(ret == NULL, "strpbrk(\"abc\", \"\") = %p, expected NULL\n", ret);

    /* Both empty - returns NULL */
    ret = p_strpbrk("", "");
    ok(ret == NULL, "strpbrk(\"\", \"\") = %p, expected NULL\n", ret);

    /* Multiple matches - returns first */
    str = "abcabc";
    ret = p_strpbrk(str, "cb");
    ok(ret == str + 1, "strpbrk(\"abcabc\", \"cb\") = %p, expected %p\n", ret, str + 1);

    /* Case sensitive */
    ret = p_strpbrk("ABCDEF", "abc");
    ok(ret == NULL, "strpbrk case = %p, expected NULL\n", ret);

    /* Control has duplicates */
    str = "xyzabc";
    ret = p_strpbrk(str, "aabb");
    ok(ret == str + 3, "strpbrk dup control = %p, expected %p\n", ret, str + 3);

    /* High byte values */
    str = "abc\x80\xFE\xFF";
    ret = p_strpbrk(str, "\xFE\xFF");
    ok(ret == str + 4, "strpbrk high bytes = %p, expected %p\n", ret, str + 4);

    /* High byte value not in control */
    str = "\x80\xFE\xFF";
    ret = p_strpbrk(str, "abc");
    ok(ret == NULL, "strpbrk high no match = %p, expected NULL\n", ret);

    /* Single char string, found */
    str = "a";
    ret = p_strpbrk(str, "a");
    ok(ret == str, "strpbrk single match = %p, expected %p\n", ret, str);

    /* Single char string, not found */
    ret = p_strpbrk("x", "abc");
    ok(ret == NULL, "strpbrk single no match = %p, expected NULL\n", ret);

    /* Space as separator */
    str = "hello world";
    ret = p_strpbrk(str, " \t\n");
    ok(ret == str + 5, "strpbrk space = %p, expected %p\n", ret, str + 5);
}

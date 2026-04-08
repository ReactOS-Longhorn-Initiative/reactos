/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strcspn
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef size_t (__cdecl *PFN_strcspn)(const char* _Str, const char* _Control);

START_TEST(strcspn)
{
    PFN_strcspn p_strcspn = (PFN_strcspn)load_function("strcspn");
    size_t ret;

    /* Basic: first rejection char */
    ret = p_strcspn("abcdef", "de");
    ok(ret == 3, "strcspn(\"abcdef\", \"de\") = %zu, expected 3\n", ret);

    /* No rejection chars found - returns string length */
    ret = p_strcspn("abcdef", "xyz");
    ok(ret == 6, "strcspn(\"abcdef\", \"xyz\") = %zu, expected 6\n", ret);

    /* First char is rejected */
    ret = p_strcspn("abcdef", "a");
    ok(ret == 0, "strcspn(\"abcdef\", \"a\") = %zu, expected 0\n", ret);

    /* Last char is rejected */
    ret = p_strcspn("abcdef", "f");
    ok(ret == 5, "strcspn(\"abcdef\", \"f\") = %zu, expected 5\n", ret);

    /* Empty string */
    ret = p_strcspn("", "abc");
    ok(ret == 0, "strcspn(\"\", \"abc\") = %zu, expected 0\n", ret);

    /* Empty control set - returns string length */
    ret = p_strcspn("abc", "");
    ok(ret == 3, "strcspn(\"abc\", \"\") = %zu, expected 3\n", ret);

    /* Both empty */
    ret = p_strcspn("", "");
    ok(ret == 0, "strcspn(\"\", \"\") = %zu, expected 0\n", ret);

    /* Single char string, not in control */
    ret = p_strcspn("x", "abc");
    ok(ret == 1, "strcspn(\"x\", \"abc\") = %zu, expected 1\n", ret);

    /* Single char string, in control */
    ret = p_strcspn("a", "abc");
    ok(ret == 0, "strcspn(\"a\", \"abc\") = %zu, expected 0\n", ret);

    /* Case sensitive */
    ret = p_strcspn("ABCdef", "abc");
    ok(ret == 6, "strcspn case = %zu, expected 6\n", ret);

    /* Control has duplicates */
    ret = p_strcspn("xyzabc", "aabb");
    ok(ret == 3, "strcspn dup control = %zu, expected 3\n", ret);

    /* High byte values in control */
    ret = p_strcspn("abc\x80" "def", "\x80\xFE");
    ok(ret == 3, "strcspn high bytes = %zu, expected 3\n", ret);

    /* High byte values not in control */
    ret = p_strcspn("\x80\xFE\xFF", "abc");
    ok(ret == 3, "strcspn high bytes no match = %zu, expected 3\n", ret);

    /* Digits and special chars */
    ret = p_strcspn("hello world!", " ");
    ok(ret == 5, "strcspn space = %zu, expected 5\n", ret);

    /* All 256 byte values in control - immediate match */
    {
        char control[256];
        for (int i = 0; i < 255; i++)
            control[i] = (char)(i + 1);
        control[255] = '\0';
        ret = p_strcspn("Hello", control);
        ok(ret == 0, "strcspn all-chars = %zu, expected 0\n", ret);
    }
}

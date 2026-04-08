/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strcmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_strcmp)(const char* _Str1, const char* _Str2);

START_TEST(strcmp)
{
    PFN_strcmp p_strcmp = (PFN_strcmp)load_function("strcmp");
    int ret;

    /* Equal strings */
    ret = p_strcmp("Hello", "Hello");
    ok(ret == 0, "strcmp(\"Hello\", \"Hello\") = %d, expected 0\n", ret);

    /* Empty strings */
    ret = p_strcmp("", "");
    ok(ret == 0, "strcmp(\"\", \"\") = %d, expected 0\n", ret);

    /* First string less than second */
    ret = p_strcmp("ABC", "ABD");
    ok(ret < 0, "strcmp(\"ABC\", \"ABD\") = %d, expected < 0\n", ret);

    /* First string greater than second */
    ret = p_strcmp("ABD", "ABC");
    ok(ret > 0, "strcmp(\"ABD\", \"ABC\") = %d, expected > 0\n", ret);

    /* First string is prefix of second */
    ret = p_strcmp("Hello", "Hello World");
    ok(ret < 0, "strcmp(\"Hello\", \"Hello World\") = %d, expected < 0\n", ret);

    /* Second string is prefix of first */
    ret = p_strcmp("Hello World", "Hello");
    ok(ret > 0, "strcmp(\"Hello World\", \"Hello\") = %d, expected > 0\n", ret);

    /* Empty vs non-empty */
    ret = p_strcmp("", "A");
    ok(ret < 0, "strcmp(\"\", \"A\") = %d, expected < 0\n", ret);

    ret = p_strcmp("A", "");
    ok(ret > 0, "strcmp(\"A\", \"\") = %d, expected > 0\n", ret);

    /* Single character comparison */
    ret = p_strcmp("A", "A");
    ok(ret == 0, "strcmp(\"A\", \"A\") = %d, expected 0\n", ret);

    ret = p_strcmp("A", "B");
    ok(ret < 0, "strcmp(\"A\", \"B\") = %d, expected < 0\n", ret);

    ret = p_strcmp("B", "A");
    ok(ret > 0, "strcmp(\"B\", \"A\") = %d, expected > 0\n", ret);

    /* Comparison must be unsigned: 0x80 > 0x7F */
    ret = p_strcmp("\x80", "\x7F");
    ok(ret > 0, "strcmp(\"\\x80\", \"\\x7F\") = %d, expected > 0\n", ret);

    ret = p_strcmp("\x7F", "\x80");
    ok(ret < 0, "strcmp(\"\\x7F\", \"\\x80\") = %d, expected < 0\n", ret);

    /* High bytes: 0xFF vs 0x01 */
    ret = p_strcmp("\xFF", "\x01");
    ok(ret > 0, "strcmp(\"\\xFF\", \"\\x01\") = %d, expected > 0\n", ret);

    ret = p_strcmp("\x01", "\xFF");
    ok(ret < 0, "strcmp(\"\\x01\", \"\\xFF\") = %d, expected < 0\n", ret);

    /* Difference in the middle */
    ret = p_strcmp("ABCXEF", "ABCDEF");
    ok(ret > 0, "strcmp mid-diff = %d, expected > 0\n", ret);

    ret = p_strcmp("ABCDEF", "ABCXEF");
    ok(ret < 0, "strcmp mid-diff2 = %d, expected < 0\n", ret);

    /* Verify exact difference value for single-char comparison */
    ret = p_strcmp("B", "A");
    ok(ret == 1, "strcmp(\"B\", \"A\") = %d, expected 1\n", ret);

    ret = p_strcmp("A", "B");
    ok(ret == -1, "strcmp(\"A\", \"B\") = %d, expected -1\n", ret);

    ret = p_strcmp("\xFF", "\x01");
    ok(ret == 1, "strcmp(\"\\xFF\", \"\\x01\") = %d, expected 1\n", ret);
}

/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strncmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_strncmp)(const char* _Str1, const char* _Str2, size_t _MaxCount);

START_TEST(strncmp)
{
    PFN_strncmp p_strncmp = (PFN_strncmp)load_function("strncmp");
    int ret;

    /* Equal strings, count covers all */
    ret = p_strncmp("Hello", "Hello", 10);
    ok(ret == 0, "strncmp(\"Hello\", \"Hello\", 10) = %d, expected 0\n", ret);

    /* Equal strings, count is exact length */
    ret = p_strncmp("Hello", "Hello", 5);
    ok(ret == 0, "strncmp(\"Hello\", \"Hello\", 5) = %d, expected 0\n", ret);

    /* Count = 0 always returns 0 */
    ret = p_strncmp("ABC", "XYZ", 0);
    ok(ret == 0, "strncmp(\"ABC\", \"XYZ\", 0) = %d, expected 0\n", ret);

    /* Difference within count */
    ret = p_strncmp("ABC", "ABD", 3);
    ok(ret < 0, "strncmp(\"ABC\", \"ABD\", 3) = %d, expected < 0\n", ret);

    ret = p_strncmp("ABD", "ABC", 3);
    ok(ret > 0, "strncmp(\"ABD\", \"ABC\", 3) = %d, expected > 0\n", ret);

    /* Difference beyond count - should be equal */
    ret = p_strncmp("ABCX", "ABCY", 3);
    ok(ret == 0, "strncmp(\"ABCX\", \"ABCY\", 3) = %d, expected 0\n", ret);

    /* First string is prefix, count covers both */
    ret = p_strncmp("Hello", "Hello World", 11);
    ok(ret < 0, "strncmp prefix = %d, expected < 0\n", ret);

    /* First string is prefix, count limited to common part */
    ret = p_strncmp("Hello", "Hello World", 5);
    ok(ret == 0, "strncmp prefix limited = %d, expected 0\n", ret);

    /* Empty strings */
    ret = p_strncmp("", "", 5);
    ok(ret == 0, "strncmp(\"\", \"\", 5) = %d, expected 0\n", ret);

    /* Empty vs non-empty */
    ret = p_strncmp("", "A", 5);
    ok(ret < 0, "strncmp(\"\", \"A\", 5) = %d, expected < 0\n", ret);

    ret = p_strncmp("A", "", 5);
    ok(ret > 0, "strncmp(\"A\", \"\", 5) = %d, expected > 0\n", ret);

    /* Empty vs non-empty, count = 0 */
    ret = p_strncmp("", "A", 0);
    ok(ret == 0, "strncmp(\"\", \"A\", 0) = %d, expected 0\n", ret);

    /* Unsigned comparison: 0x80 > 0x7F */
    ret = p_strncmp("\x80", "\x7F", 1);
    ok(ret > 0, "strncmp(\"\\x80\", \"\\x7F\", 1) = %d, expected > 0\n", ret);

    ret = p_strncmp("\x7F", "\x80", 1);
    ok(ret < 0, "strncmp(\"\\x7F\", \"\\x80\", 1) = %d, expected < 0\n", ret);

    /* Verify exact difference value */
    ret = p_strncmp("B", "A", 1);
    ok(ret == 1, "strncmp(\"B\", \"A\", 1) = %d, expected 1\n", ret);

    ret = p_strncmp("A", "B", 1);
    ok(ret == -1, "strncmp(\"A\", \"B\", 1) = %d, expected -1\n", ret);

    /* Count = 1 on longer strings, first char equal */
    ret = p_strncmp("ABCDEF", "AXYZ", 1);
    ok(ret == 0, "strncmp count=1 same-first = %d, expected 0\n", ret);

    /* Null terminator encountered before count */
    ret = p_strncmp("AB", "AB", 100);
    ok(ret == 0, "strncmp short-strings-big-count = %d, expected 0\n", ret);
}

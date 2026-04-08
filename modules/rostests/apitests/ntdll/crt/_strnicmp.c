/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _strnicmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__strnicmp)(const char* _String1, const char* _String2, size_t _MaxCount);

START_TEST(_strnicmp)
{
    PFN__strnicmp p_strnicmp = (PFN__strnicmp)load_function("_strnicmp");
    int ret;

    /* Equal strings, count covers all */
    ret = p_strnicmp("Hello", "Hello", 10);
    ok(ret == 0, "_strnicmp(\"Hello\", \"Hello\", 10) = %d, expected 0\n", ret);

    /* Case-insensitive equality, count covers all */
    ret = p_strnicmp("Hello", "HELLO", 10);
    ok(ret == 0, "_strnicmp(\"Hello\", \"HELLO\", 10) = %d, expected 0\n", ret);

    ret = p_strnicmp("HELLO", "hello", 5);
    ok(ret == 0, "_strnicmp(\"HELLO\", \"hello\", 5) = %d, expected 0\n", ret);

    /* Count = 0 always returns 0 */
    ret = p_strnicmp("ABC", "XYZ", 0);
    ok(ret == 0, "_strnicmp(\"ABC\", \"XYZ\", 0) = %d, expected 0\n", ret);

    /* Difference within count */
    ret = p_strnicmp("ABC", "abd", 3);
    ok(ret < 0, "_strnicmp(\"ABC\", \"abd\", 3) = %d, expected < 0\n", ret);

    ret = p_strnicmp("ABD", "abc", 3);
    ok(ret > 0, "_strnicmp(\"ABD\", \"abc\", 3) = %d, expected > 0\n", ret);

    /* Difference beyond count - should be equal */
    ret = p_strnicmp("ABCX", "abcy", 3);
    ok(ret == 0, "_strnicmp beyond-count = %d, expected 0\n", ret);

    /* First string is prefix, count covers both */
    ret = p_strnicmp("Hello", "HELLO WORLD", 11);
    ok(ret < 0, "_strnicmp prefix = %d, expected < 0\n", ret);

    /* First string is prefix, count limited to common part */
    ret = p_strnicmp("Hello", "HELLO WORLD", 5);
    ok(ret == 0, "_strnicmp prefix limited = %d, expected 0\n", ret);

    /* Empty strings */
    ret = p_strnicmp("", "", 5);
    ok(ret == 0, "_strnicmp(\"\", \"\", 5) = %d, expected 0\n", ret);

    /* Empty vs non-empty */
    ret = p_strnicmp("", "A", 5);
    ok(ret < 0, "_strnicmp(\"\", \"A\", 5) = %d, expected < 0\n", ret);

    ret = p_strnicmp("A", "", 5);
    ok(ret > 0, "_strnicmp(\"A\", \"\", 5) = %d, expected > 0\n", ret);

    /* Empty vs non-empty, count = 0 */
    ret = p_strnicmp("", "A", 0);
    ok(ret == 0, "_strnicmp(\"\", \"A\", 0) = %d, expected 0\n", ret);

    /* Case-insensitive single character */
    ret = p_strnicmp("A", "a", 1);
    ok(ret == 0, "_strnicmp(\"A\", \"a\", 1) = %d, expected 0\n", ret);

    ret = p_strnicmp("Z", "z", 1);
    ok(ret == 0, "_strnicmp(\"Z\", \"z\", 1) = %d, expected 0\n", ret);

    /* Verify exact difference value */
    ret = p_strnicmp("b", "a", 1);
    ok(ret == 1, "_strnicmp(\"b\", \"a\", 1) = %d, expected 1\n", ret);

    ret = p_strnicmp("a", "b", 1);
    ok(ret == -1, "_strnicmp(\"a\", \"b\", 1) = %d, expected -1\n", ret);

    /* High bytes are not lowered */
    ret = p_strnicmp("\x80", "\x7F", 1);
    ok(ret > 0, "_strnicmp(\"\\x80\", \"\\x7F\", 1) = %d, expected > 0\n", ret);

    ret = p_strnicmp("\x7F", "\x80", 1);
    ok(ret < 0, "_strnicmp(\"\\x7F\", \"\\x80\", 1) = %d, expected < 0\n", ret);

    /* Count = 1 on longer strings, first char equal (case-insensitive) */
    ret = p_strnicmp("ABCDEF", "axyz", 1);
    ok(ret == 0, "_strnicmp count=1 same-first = %d, expected 0\n", ret);

    /* Null terminator encountered before count */
    ret = p_strnicmp("AB", "ab", 100);
    ok(ret == 0, "_strnicmp short-strings-big-count = %d, expected 0\n", ret);

    /* Mixed case comparison */
    ret = p_strnicmp("aBcDeF", "AbCdEf", 6);
    ok(ret == 0, "_strnicmp mixed case = %d, expected 0\n", ret);

    /* Characters just outside A-Z range are not lowered */
    ret = p_strnicmp("@", "`", 1);
    ok(ret < 0, "_strnicmp(\"@\", \"`\", 1) = %d, expected < 0\n", ret);

    ret = p_strnicmp("[", "{", 1);
    ok(ret < 0, "_strnicmp(\"[\", \"{\", 1) = %d, expected < 0\n", ret);

    /* '_' (0x5F) is between upper case 'A'-'Z' (0x41-0x5A) and lower case
       'a'-'z' (0x61-0x7A). CRT uses lower case folding, so 'A' becomes 'a'
       (0x61), which is greater than '_' (0x5F). This is the opposite of Rtl
       functions that use upper case folding. */
    ret = p_strnicmp("_", "A", 1);
    ok(ret < 0, "_strnicmp(\"_\", \"A\", 1) = %d, expected < 0\n", ret);
    ret = p_strnicmp("A", "_", 1);
    ok(ret > 0, "_strnicmp(\"A\", \"_\", 1) = %d, expected > 0\n", ret);
    ret = p_strnicmp("_", "a", 1);
    ok(ret < 0, "_strnicmp(\"_\", \"a\", 1) = %d, expected < 0\n", ret);
    ret = p_strnicmp("a", "_", 1);
    ok(ret > 0, "_strnicmp(\"a\", \"_\", 1) = %d, expected > 0\n", ret);
}

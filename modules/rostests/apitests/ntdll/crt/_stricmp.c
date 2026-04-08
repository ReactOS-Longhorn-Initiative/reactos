/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _stricmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__stricmp)(const char* _String1, const char* _String2);

START_TEST(_stricmp)
{
    PFN__stricmp p_stricmp = (PFN__stricmp)load_function("_stricmp");
    int ret;

    /* Equal strings */
    ret = p_stricmp("Hello", "Hello");
    ok(ret == 0, "_stricmp(\"Hello\", \"Hello\") = %d, expected 0\n", ret);

    /* Case-insensitive equality */
    ret = p_stricmp("Hello", "HELLO");
    ok(ret == 0, "_stricmp(\"Hello\", \"HELLO\") = %d, expected 0\n", ret);

    ret = p_stricmp("HELLO", "hello");
    ok(ret == 0, "_stricmp(\"HELLO\", \"hello\") = %d, expected 0\n", ret);

    ret = p_stricmp("hello", "HELLO");
    ok(ret == 0, "_stricmp(\"hello\", \"HELLO\") = %d, expected 0\n", ret);

    /* Empty strings */
    ret = p_stricmp("", "");
    ok(ret == 0, "_stricmp(\"\", \"\") = %d, expected 0\n", ret);

    /* First string less than second (case-insensitive) */
    ret = p_stricmp("ABC", "abd");
    ok(ret < 0, "_stricmp(\"ABC\", \"abd\") = %d, expected < 0\n", ret);

    /* First string greater than second (case-insensitive) */
    ret = p_stricmp("ABD", "abc");
    ok(ret > 0, "_stricmp(\"ABD\", \"abc\") = %d, expected > 0\n", ret);

    /* First string is prefix of second */
    ret = p_stricmp("Hello", "Hello World");
    ok(ret < 0, "_stricmp prefix = %d, expected < 0\n", ret);

    /* Second string is prefix of first */
    ret = p_stricmp("Hello World", "Hello");
    ok(ret > 0, "_stricmp prefix2 = %d, expected > 0\n", ret);

    /* Empty vs non-empty */
    ret = p_stricmp("", "A");
    ok(ret < 0, "_stricmp(\"\", \"A\") = %d, expected < 0\n", ret);

    ret = p_stricmp("A", "");
    ok(ret > 0, "_stricmp(\"A\", \"\") = %d, expected > 0\n", ret);

    /* Single character case-insensitive */
    ret = p_stricmp("A", "a");
    ok(ret == 0, "_stricmp(\"A\", \"a\") = %d, expected 0\n", ret);

    ret = p_stricmp("a", "A");
    ok(ret == 0, "_stricmp(\"a\", \"A\") = %d, expected 0\n", ret);

    ret = p_stricmp("Z", "z");
    ok(ret == 0, "_stricmp(\"Z\", \"z\") = %d, expected 0\n", ret);

    /* Verify exact difference value */
    ret = p_stricmp("b", "a");
    ok(ret == 1, "_stricmp(\"b\", \"a\") = %d, expected 1\n", ret);

    ret = p_stricmp("a", "b");
    ok(ret == -1, "_stricmp(\"a\", \"b\") = %d, expected -1\n", ret);

    ret = p_stricmp("B", "A");
    ok(ret == 1, "_stricmp(\"B\", \"A\") = %d, expected 1\n", ret);

    /* Only ASCII letters are lowered, high bytes are unchanged */
    ret = p_stricmp("\xFF", "\x01");
    ok(ret == 0xFE, "_stricmp(\"\\xFF\", \"\\x01\") = %d, expected 254\n", ret);

    ret = p_stricmp("\x80", "\x7F");
    ok(ret > 0, "_stricmp(\"\\x80\", \"\\x7F\") = %d, expected > 0\n", ret);

    /* Mixed case in the middle */
    ret = p_stricmp("aBcDeF", "AbCdEf");
    ok(ret == 0, "_stricmp mixed case = %d, expected 0\n", ret);

    /* Difference in the middle, case-insensitive */
    ret = p_stricmp("ABCXEF", "abcdef");
    ok(ret > 0, "_stricmp mid-diff = %d, expected > 0\n", ret);

    ret = p_stricmp("abcdef", "ABCXEF");
    ok(ret < 0, "_stricmp mid-diff2 = %d, expected < 0\n", ret);

    /* Characters just outside A-Z range are not lowered */
    ret = p_stricmp("@", "`");
    ok(ret < 0, "_stricmp(\"@\", \"`\") = %d, expected < 0\n", ret);

    ret = p_stricmp("[", "{");
    ok(ret < 0, "_stricmp(\"[\", \"{\") = %d, expected < 0\n", ret);

    /* '_' (0x5F) is between upper case 'A'-'Z' (0x41-0x5A) and lower case
       'a'-'z' (0x61-0x7A). CRT uses lower case folding, so 'A' becomes 'a'
       (0x61), which is greater than '_' (0x5F). This is the opposite of Rtl
       functions that use upper case folding. */
    ret = p_stricmp("_", "A");
    ok(ret < 0, "_stricmp(\"_\", \"A\") = %d, expected < 0\n", ret);
    ret = p_stricmp("A", "_");
    ok(ret > 0, "_stricmp(\"A\", \"_\") = %d, expected > 0\n", ret);
    ret = p_stricmp("_", "a");
    ok(ret < 0, "_stricmp(\"_\", \"a\") = %d, expected < 0\n", ret);
    ret = p_stricmp("a", "_");
    ok(ret > 0, "_stricmp(\"a\", \"_\") = %d, expected > 0\n", ret);
}

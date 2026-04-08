/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _memicmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN__memicmp)(const void* _Buf1, const void* _Buf2, size_t _Size);

START_TEST(_memicmp)
{
    PFN__memicmp p_memicmp = (PFN__memicmp)load_function("_memicmp");
    int ret;

    /* Equal buffers */
    ret = p_memicmp("Hello", "Hello", 5);
    ok(ret == 0, "_memicmp(\"Hello\", \"Hello\", 5) = %d, expected 0\n", ret);

    /* Case-insensitive equality */
    ret = p_memicmp("Hello", "HELLO", 5);
    ok(ret == 0, "_memicmp(\"Hello\", \"HELLO\", 5) = %d, expected 0\n", ret);

    ret = p_memicmp("HELLO", "hello", 5);
    ok(ret == 0, "_memicmp(\"HELLO\", \"hello\", 5) = %d, expected 0\n", ret);

    ret = p_memicmp("hello", "HELLO", 5);
    ok(ret == 0, "_memicmp(\"hello\", \"HELLO\", 5) = %d, expected 0\n", ret);

    /* Count = 0 always returns 0 */
    ret = p_memicmp("ABC", "xyz", 0);
    ok(ret == 0, "_memicmp(\"ABC\", \"xyz\", 0) = %d, expected 0\n", ret);

    /* First buffer less than second (case-insensitive) */
    ret = p_memicmp("ABC", "abd", 3);
    ok(ret < 0, "_memicmp(\"ABC\", \"abd\") = %d, expected < 0\n", ret);

    /* First buffer greater than second (case-insensitive) */
    ret = p_memicmp("ABD", "abc", 3);
    ok(ret > 0, "_memicmp(\"ABD\", \"abc\") = %d, expected > 0\n", ret);

    /* Difference beyond count - should be equal */
    ret = p_memicmp("ABCxyz", "abcDEF", 3);
    ok(ret == 0, "_memicmp count limited = %d, expected 0\n", ret);

    /* Single character case-insensitive */
    ret = p_memicmp("A", "a", 1);
    ok(ret == 0, "_memicmp(\"A\", \"a\", 1) = %d, expected 0\n", ret);

    ret = p_memicmp("a", "A", 1);
    ok(ret == 0, "_memicmp(\"a\", \"A\", 1) = %d, expected 0\n", ret);

    ret = p_memicmp("Z", "z", 1);
    ok(ret == 0, "_memicmp(\"Z\", \"z\", 1) = %d, expected 0\n", ret);

    /* Verify exact difference value */
    ret = p_memicmp("b", "a", 1);
    ok(ret == 1, "_memicmp(\"b\", \"a\", 1) = %d, expected 1\n", ret);

    ret = p_memicmp("a", "b", 1);
    ok(ret == -1, "_memicmp(\"a\", \"b\", 1) = %d, expected -1\n", ret);

    ret = p_memicmp("B", "A", 1);
    ok(ret == 1, "_memicmp(\"B\", \"A\", 1) = %d, expected 1\n", ret);

    /* Only ASCII letters are lowered, high bytes are unchanged */
    ret = p_memicmp("\xFF", "\x01", 1);
    ok(ret == 0xFE, "_memicmp(\"\\xFF\", \"\\x01\", 1) = %d, expected 254\n", ret);

    ret = p_memicmp("\x80", "\x7F", 1);
    ok(ret > 0, "_memicmp(\"\\x80\", \"\\x7F\", 1) = %d, expected > 0\n", ret);

    /* Mixed case in the middle */
    ret = p_memicmp("aBcDeF", "AbCdEf", 6);
    ok(ret == 0, "_memicmp mixed case = %d, expected 0\n", ret);

    /* Difference in the middle, case-insensitive */
    ret = p_memicmp("ABCXEF", "abcdef", 6);
    ok(ret > 0, "_memicmp mid-diff = %d, expected > 0\n", ret);

    ret = p_memicmp("abcdef", "ABCXEF", 6);
    ok(ret < 0, "_memicmp mid-diff2 = %d, expected < 0\n", ret);

    /* Characters just outside A-Z range are not lowered */
    ret = p_memicmp("@", "`", 1);
    ok(ret < 0, "_memicmp(\"@\", \"`\") = %d, expected < 0\n", ret);

    ret = p_memicmp("[", "{", 1);
    ok(ret < 0, "_memicmp(\"[\", \"{\") = %d, expected < 0\n", ret);

    /* '_' (0x5F) is between upper case 'A'-'Z' (0x41-0x5A) and lower case
       'a'-'z' (0x61-0x7A). CRT uses lower case folding, so 'A' becomes 'a'
       (0x61), which is greater than '_' (0x5F). This is the opposite of Rtl
       functions that use upper case folding. */
    ret = p_memicmp("_", "A", 1);
    ok(ret < 0, "_memicmp(\"_\", \"A\") = %d, expected < 0\n", ret);
    ret = p_memicmp("A", "_", 1);
    ok(ret > 0, "_memicmp(\"A\", \"_\") = %d, expected > 0\n", ret);
    ret = p_memicmp("_", "a", 1);
    ok(ret < 0, "_memicmp(\"_\", \"a\") = %d, expected < 0\n", ret);
    ret = p_memicmp("a", "_", 1);
    ok(ret > 0, "_memicmp(\"a\", \"_\") = %d, expected > 0\n", ret);

    /* Null bytes in the middle - _memicmp continues past them */
    {
        const unsigned char buf1[] = { 'A', 0x00, 'C' };
        const unsigned char buf2[] = { 'a', 0x00, 'D' };
        ret = p_memicmp(buf1, buf2, 3);
        ok(ret < 0, "_memicmp with embedded nulls = %d, expected < 0\n", ret);
    }
}

/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for memcmp
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef int (__cdecl *PFN_memcmp)(const void* _Buf1, const void* _Buf2, size_t _Size);

START_TEST(memcmp)
{
    PFN_memcmp p_memcmp = (PFN_memcmp)load_function("memcmp");
    int ret;

    /* Equal buffers */
    ret = p_memcmp("Hello", "Hello", 5);
    ok(ret == 0, "memcmp(\"Hello\", \"Hello\", 5) = %d, expected 0\n", ret);

    /* Count = 0 always returns 0 */
    ret = p_memcmp("ABC", "XYZ", 0);
    ok(ret == 0, "memcmp(\"ABC\", \"XYZ\", 0) = %d, expected 0\n", ret);

    /* First buffer less than second */
    ret = p_memcmp("ABC", "ABD", 3);
    ok(ret < 0, "memcmp(\"ABC\", \"ABD\", 3) = %d, expected < 0\n", ret);

    /* First buffer greater than second */
    ret = p_memcmp("ABD", "ABC", 3);
    ok(ret > 0, "memcmp(\"ABD\", \"ABC\", 3) = %d, expected > 0\n", ret);

    /* Difference beyond count - should be equal */
    ret = p_memcmp("ABCX", "ABCY", 3);
    ok(ret == 0, "memcmp(\"ABCX\", \"ABCY\", 3) = %d, expected 0\n", ret);

    /* Difference at first byte */
    ret = p_memcmp("X", "A", 1);
    ok(ret > 0, "memcmp(\"X\", \"A\", 1) = %d, expected > 0\n", ret);

    ret = p_memcmp("A", "X", 1);
    ok(ret < 0, "memcmp(\"A\", \"X\", 1) = %d, expected < 0\n", ret);

    /* Difference at the end */
    ret = p_memcmp("ABCDE", "ABCDF", 5);
    ok(ret < 0, "memcmp(\"ABCDE\", \"ABCDF\", 5) = %d, expected < 0\n", ret);

    ret = p_memcmp("ABCDF", "ABCDE", 5);
    ok(ret > 0, "memcmp(\"ABCDF\", \"ABCDE\", 5) = %d, expected > 0\n", ret);

    /* Comparison must be unsigned: 0x80 > 0x7F */
    ret = p_memcmp("\x80", "\x7F", 1);
    ok(ret > 0, "memcmp(\"\\x80\", \"\\x7F\", 1) = %d, expected > 0\n", ret);

    ret = p_memcmp("\x7F", "\x80", 1);
    ok(ret < 0, "memcmp(\"\\x7F\", \"\\x80\", 1) = %d, expected < 0\n", ret);

    /* High bytes: 0xFF vs 0x01 */
    ret = p_memcmp("\xFF", "\x01", 1);
    ok(ret > 0, "memcmp(\"\\xFF\", \"\\x01\", 1) = %d, expected > 0\n", ret);

    ret = p_memcmp("\x01", "\xFF", 1);
    ok(ret < 0, "memcmp(\"\\x01\", \"\\xFF\", 1) = %d, expected < 0\n", ret);

    /* Null bytes in the middle - memcmp continues past them */
    {
        const unsigned char buf1[] = { 'A', 'B', 0x00, 'C', 'D' };
        const unsigned char buf2[] = { 'A', 'B', 0x00, 'C', 'E' };
        ret = p_memcmp(buf1, buf2, 5);
        ok(ret < 0, "memcmp with embedded nulls = %d, expected < 0\n", ret);
    }

    {
        const unsigned char buf1[] = { 'A', 'B', 0x00, 'C', 'E' };
        const unsigned char buf2[] = { 'A', 'B', 0x00, 'C', 'D' };
        ret = p_memcmp(buf1, buf2, 5);
        ok(ret > 0, "memcmp with embedded nulls2 = %d, expected > 0\n", ret);
    }

    /* Verify exact return values: normalized -1, 0, 1 */
    ret = p_memcmp("B", "A", 1);
    ok(ret == 1, "memcmp(\"B\", \"A\", 1) = %d, expected 1\n", ret);

    ret = p_memcmp("A", "B", 1);
    ok(ret == -1, "memcmp(\"A\", \"B\", 1) = %d, expected -1\n", ret);

    ret = p_memcmp("\xFF", "\x01", 1);
    ok(ret == 1, "memcmp(\"\\xFF\", \"\\x01\", 1) = %d, expected 1\n", ret);

    ret = p_memcmp("\x01", "\xFF", 1);
    ok(ret == -1, "memcmp(\"\\x01\", \"\\xFF\", 1) = %d, expected -1\n", ret);

    /* Difference in the middle of buffer */
    ret = p_memcmp("ABCXEF", "ABCDEF", 6);
    ok(ret == 1, "memcmp mid-diff = %d, expected 1\n", ret);

    ret = p_memcmp("ABCDEF", "ABCXEF", 6);
    ok(ret == -1, "memcmp mid-diff2 = %d, expected -1\n", ret);

    /* Single byte equal */
    ret = p_memcmp("A", "A", 1);
    ok(ret == 0, "memcmp(\"A\", \"A\", 1) = %d, expected 0\n", ret);
}

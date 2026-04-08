/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strncat
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef char* (__cdecl *PFN_strncat)(char* _Destination, const char* _Source, size_t _Count);

START_TEST(strncat)
{
    PFN_strncat p_strncat = (PFN_strncat)load_function("strncat");
    char dst[64];
    char *ret;

    /* Basic: count >= source length */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat(dst, " World", 10);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello World\0\xAA", 13) == 0, "strncat basic content mismatch\n");

    /* Count limits the copy */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat(dst, " World", 3);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello Wo\0\xAA", 10) == 0, "strncat limited content mismatch\n");

    /* Count = 0 appends nothing */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat(dst, " World", 0);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strncat zero-count content mismatch\n");

    /* Append to empty string */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = '\0';
    ret = p_strncat(dst, "Test", 10);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Test\0\xAA", 6) == 0, "strncat to-empty content mismatch\n");

    /* Append empty string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat(dst, "", 10);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strncat empty-append content mismatch\n");

    /* Source shorter than count */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "AB", 3);
    ret = p_strncat(dst, "CD", 10);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "ABCD\0\xAA", 6) == 0, "strncat short-src content mismatch\n");

    /* Count exactly matches source length */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strncat(dst, "XYZ", 3);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "HiXYZ\0\xAA", 7) == 0, "strncat exact-count content mismatch\n");

    /* Special characters */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "\x80", 2);
    ret = p_strncat(dst, "\xFE\xFF", 2);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "\x80\xFE\xFF\0\xAA", 5) == 0, "strncat special content mismatch\n");

    /* Both empty with count 0 */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = '\0';
    ret = p_strncat(dst, "", 0);
    ok(ret == dst, "strncat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "\0\xAA", 2) == 0, "strncat both-empty content mismatch\n");
}

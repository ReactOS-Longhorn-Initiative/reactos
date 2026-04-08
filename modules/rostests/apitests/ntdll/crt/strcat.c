/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strcat
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef char* (__cdecl *PFN_strcat)(char* _Destination, const char* _Source);

START_TEST(strcat)
{
    PFN_strcat p_strcat = (PFN_strcat)load_function("strcat");
    char dst[64];
    char *ret;

    /* Basic concatenation */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strcat(dst, " World");
    ok(ret == dst, "strcat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello World\0\xAA", 13) == 0, "strcat basic content mismatch\n");

    /* Append to empty string */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = '\0';
    ret = p_strcat(dst, "Test");
    ok(ret == dst, "strcat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Test\0\xAA", 6) == 0, "strcat to-empty content mismatch\n");

    /* Append empty string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strcat(dst, "");
    ok(ret == dst, "strcat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strcat empty-append content mismatch\n");

    /* Both empty */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = '\0';
    ret = p_strcat(dst, "");
    ok(ret == dst, "strcat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "\0\xAA", 2) == 0, "strcat both-empty content mismatch\n");

    /* String with special characters */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "A\x80\xFE", 4);
    ret = p_strcat(dst, "\t\n\xFF");
    ok(ret == dst, "strcat returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "A\x80\xFE\t\n\xFF\0\xAA", 8) == 0, "strcat special chars content mismatch\n");

    /* Multiple concatenations */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = '\0';
    p_strcat(dst, "abc");
    p_strcat(dst, "def");
    p_strcat(dst, "ghi");
    ok(memcmp(dst, "abcdefghi\0\xAA", 11) == 0, "strcat multi content mismatch\n");
}

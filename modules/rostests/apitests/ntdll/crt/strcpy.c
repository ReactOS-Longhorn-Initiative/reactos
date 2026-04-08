/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strcpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef char* (__cdecl *PFN_strcpy)(char* _Destination, const char* _Source);

START_TEST(strcpy)
{
    PFN_strcpy p_strcpy = (PFN_strcpy)load_function("strcpy");
    char dst[64];
    char *ret;

    /* Basic string copy */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy(dst, "Hello");
    ok(ret == dst, "strcpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strcpy basic content mismatch\n");

    /* Single character */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy(dst, "A");
    ok(ret == dst, "strcpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "A\0\xAA", 3) == 0, "strcpy single char content mismatch\n");

    /* Empty string */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy(dst, "");
    ok(ret == dst, "strcpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "\0\xAA", 2) == 0, "strcpy empty content mismatch\n");

    /* String with special characters */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy(dst, "A\x80\xFE\t\n");
    ok(ret == dst, "strcpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "A\x80\xFE\t\n\0\xAA", 7) == 0, "strcpy special chars content mismatch\n");

    /* Overwrite existing content */
    memset(dst, 0xAA, sizeof(dst));
    p_strcpy(dst, "LongString12345");
    p_strcpy(dst, "Short");
    ok(memcmp(dst, "Short\0ring12345\0", 16) == 0, "strcpy overwrite content mismatch\n");
    ok((unsigned char)dst[16] == 0xAA, "strcpy overwrite: filler corrupted\n");

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello\0", 6);
    ret = p_strcpy(dst, dst);
    ok(ret == dst, "strcpy self returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strcpy self content mismatch\n");

    /* Overlapping: src at positive offset from dst (dst < src) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello World\0", 12);
    ret = p_strcpy(dst, dst + 6);
    ok(ret == dst, "strcpy fwd-overlap returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "World\0World\0\xAA", 13) == 0, "strcpy fwd-overlap content mismatch\n");
}

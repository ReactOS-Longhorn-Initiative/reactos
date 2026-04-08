/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strncpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>

typedef char* (__cdecl *PFN_strncpy)(char* _Destination, const char* _Source, size_t _Count);

START_TEST(strncpy)
{
    PFN_strncpy p_strncpy = (PFN_strncpy)load_function("strncpy");
    char dst[64];
    char *ret;

    /* Source shorter than count - pads with nulls */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy(dst, "Hi", 5);
    ok(ret == dst, "strncpy returned %p, expected %p\n", ret, dst);
    ok(dst[0] == 'H', "strncpy dst[0] = 0x%02x\n", (unsigned char)dst[0]);
    ok(dst[1] == 'i', "strncpy dst[1] = 0x%02x\n", (unsigned char)dst[1]);
    ok(dst[2] == '\0', "strncpy dst[2] = 0x%02x, expected 0\n", (unsigned char)dst[2]);
    ok(dst[3] == '\0', "strncpy dst[3] = 0x%02x, expected 0 (padding)\n", (unsigned char)dst[3]);
    ok(dst[4] == '\0', "strncpy dst[4] = 0x%02x, expected 0 (padding)\n", (unsigned char)dst[4]);
    ok((unsigned char)dst[5] == 0xAA, "strncpy wrote past count\n");

    /* Source longer than count - no null terminator */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy(dst, "Hello World", 5);
    ok(ret == dst, "strncpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello", 5) == 0, "strncpy content mismatch\n");
    ok((unsigned char)dst[5] == 0xAA, "strncpy wrote past count\n");

    /* Source exactly count length - no null terminator */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy(dst, "ABCDE", 5);
    ok(ret == dst, "strncpy returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "ABCDE", 5) == 0, "strncpy exact content mismatch\n");
    ok((unsigned char)dst[5] == 0xAA, "strncpy exact wrote past count\n");

    /* Count of zero - copies nothing */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy(dst, "Hello", 0);
    ok(ret == dst, "strncpy(count=0) returned %p, expected %p\n", ret, dst);
    ok((unsigned char)dst[0] == 0xAA, "strncpy(count=0) modified dst\n");

    /* Empty source with count > 0 - fills with nulls */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy(dst, "", 3);
    ok(ret == dst, "strncpy returned %p, expected %p\n", ret, dst);
    ok(dst[0] == '\0', "strncpy empty dst[0] = 0x%02x\n", (unsigned char)dst[0]);
    ok(dst[1] == '\0', "strncpy empty dst[1] = 0x%02x\n", (unsigned char)dst[1]);
    ok(dst[2] == '\0', "strncpy empty dst[2] = 0x%02x\n", (unsigned char)dst[2]);
    ok((unsigned char)dst[3] == 0xAA, "strncpy empty wrote past count\n");

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello\0", 6);
    ret = p_strncpy(dst, dst, 5);
    ok(ret == dst, "strncpy self returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "Hello", 5) == 0, "strncpy self content mismatch\n");

    /* Overlapping: src at positive offset from dst */
    memcpy(dst, "Hello World\0", 12);
    ret = p_strncpy(dst, dst + 6, 5);
    ok(ret == dst, "strncpy fwd-overlap returned %p, expected %p\n", ret, dst);
    ok(memcmp(dst, "World", 5) == 0, "strncpy fwd-overlap content mismatch\n");

    /* Overlapping: dst at positive offset from src */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello\0", 6);
    ret = p_strncpy(dst + 2, dst, 8);
    ok(ret == dst + 2, "strcpy_s bwd-overlap returned %p, expected dst + 2 (%p)\n", ret, dst + 2);
    ok(memcmp(dst, "HeHello\0\0\0\xAA\xAA", 12) == 0, "strncpy bwd-overlap content mismatch\n");
}

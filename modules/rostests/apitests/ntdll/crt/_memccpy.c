/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _memccpy
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>

typedef void* (__cdecl *PFN__memccpy)(void* _Dst, const void* _Src, int _Val, size_t _MaxCount);

START_TEST(_memccpy)
{
    PFN__memccpy p_memccpy = (PFN__memccpy)load_function("_memccpy");
    char dst[64];
    void *ret;

    /* Basic copy until character found */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memccpy(dst, "Hello World", ' ', 11);
    ok(ret == dst + 6, "_memccpy returned %p, expected %p\n", ret, dst + 6);
    ok(memcmp(dst, "Hello ", 6) == 0, "_memccpy content mismatch\n");
    ok((unsigned char)dst[6] == 0xAA, "_memccpy wrote past stop char\n");

    /* Character not found within count - copies count bytes, returns NULL */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memccpy(dst, "Hello", 'X', 5);
    ok(ret == NULL, "_memccpy returned %p, expected NULL\n", ret);
    ok(memcmp(dst, "Hello", 5) == 0, "_memccpy content mismatch\n");
    ok((unsigned char)dst[5] == 0xAA, "_memccpy wrote past count\n");

    /* Stop character is the first byte */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memccpy(dst, "ABCDE", 'A', 5);
    ok(ret == dst + 1, "_memccpy returned %p, expected %p\n", ret, dst + 1);
    ok(dst[0] == 'A', "_memccpy dst[0] = 0x%02x\n", (unsigned char)dst[0]);
    ok((unsigned char)dst[1] == 0xAA, "_memccpy wrote past stop char\n");

    /* Stop character is the last byte within count */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memccpy(dst, "ABCDE", 'E', 5);
    ok(ret == dst + 5, "_memccpy returned %p, expected %p\n", ret, dst + 5);
    ok(memcmp(dst, "ABCDE", 5) == 0, "_memccpy content mismatch\n");
    ok((unsigned char)dst[5] == 0xAA, "_memccpy wrote past stop char\n");

    /* Zero count - returns NULL, copies nothing */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memccpy(dst, "Hello", 'H', 0);
    ok(ret == NULL, "_memccpy(count=0) returned %p, expected NULL\n", ret);
    ok((unsigned char)dst[0] == 0xAA, "_memccpy(count=0) modified dst\n");

    /* Stop character matched as unsigned char */
    memset(dst, 0xAA, sizeof(dst));
    char src_high[] = { '\x80', '\xFE', '\xFF', 0 };
    ret = p_memccpy(dst, src_high, 0xFE, 3);
    ok(ret == dst + 2, "_memccpy returned %p, expected %p\n", ret, dst + 2);
    ok((unsigned char)dst[0] == 0x80, "_memccpy dst[0] = 0x%02x\n", (unsigned char)dst[0]);
    ok((unsigned char)dst[1] == 0xFE, "_memccpy dst[1] = 0x%02x\n", (unsigned char)dst[1]);
    ok((unsigned char)dst[2] == 0xAA, "_memccpy wrote past stop char\n");

    /* Stop character as negative int (0x1FE truncated to 0xFE) */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memccpy(dst, src_high, 0x1FE, 3);
    ok(ret == dst + 2, "_memccpy(0x1FE) returned %p, expected %p\n", ret, dst + 2);
    ok(((unsigned char)dst[0] == 0x80) && ((unsigned char)dst[1] == 0xFE) &&
       ((unsigned char)dst[2] == 0xAA),
       "_memccpy(0x1FE) content mismatch\n");

    /* Copy with null bytes in source */
    memset(dst, 0xAA, sizeof(dst));
    char src_null[] = { 'A', '\0', 'B', '\0', 'C' };
    ret = p_memccpy(dst, src_null, 'B', 5);
    ok(ret == dst + 3, "_memccpy returned %p, expected %p\n", ret, dst + 3);
    ok(memcmp(dst, src_null, 3) == 0, "_memccpy content mismatch with nulls\n");

    /* Overlapping: dst == src (copy to self, stop char found) */
    memcpy(dst, "ABCDE", 5);
    ret = p_memccpy(dst, dst, 'C', 5);
    ok(ret == dst + 3, "_memccpy self returned %p, expected %p\n", ret, dst + 3);
    ok(memcmp(dst, "ABC", 3) == 0, "_memccpy self content mismatch\n");

    /* Overlapping: src at positive offset from dst (dst < src, forward overlap) */
    memcpy(dst, "0123456789", 10);
    ret = p_memccpy(dst, dst + 3, '6', 7);
    ok(ret == dst + 4, "_memccpy fwd-overlap returned %p, expected %p\n", ret, dst + 4);
    ok(memcmp(dst, "3456", 4) == 0, "_memccpy fwd-overlap content mismatch\n");

    /* Overlapping: dst at positive offset from src (src < dst, backward overlap) */
    memcpy(dst, "ABCDEFGHIJ", 10);
    ret = p_memccpy(dst + 3, dst, 'C', 5);
    ok(ret == dst + 6, "_memccpy bwd-overlap returned %p, expected %p\n", ret, dst + 6);
    ok(memcmp(dst + 3, "ABC", 3) == 0, "_memccpy bwd-overlap content mismatch\n");
}

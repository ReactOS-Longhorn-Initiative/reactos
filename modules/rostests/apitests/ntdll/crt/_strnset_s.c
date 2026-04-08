/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _strnset_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN__strnset_s)(char* _String, size_t _SizeInBytes, int _Value, size_t _MaxCount);

START_TEST(_strnset_s)
{
    PFN__strnset_s p_strnset_s = (PFN__strnset_s)load_function("_strnset_s");
    char buf[64];
    errno_t ret;

    if (p_strnset_s == NULL)
    {
        skip("Function _strnset_s not available\n");
        return;
    }

    /* Basic: count < string length */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "Hello\0", 6);
    ret = p_strnset_s(buf, sizeof(buf), 'X', 3);
    ok(ret == 0, "_strnset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, "XXXlo\0\xAA", 7) == 0, "_strnset_s basic content mismatch\n");

    /* Count == string length - fills entire string */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "Hello\0", 6);
    ret = p_strnset_s(buf, sizeof(buf), 'X', 5);
    ok(ret == 0, "_strnset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, "XXXXX\0\xAA", 7) == 0, "_strnset_s full content mismatch\n");

    /* Count > string length - stops at null terminator */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "Hi\0", 3);
    ret = p_strnset_s(buf, sizeof(buf), 'X', 10);
    ok(ret == 0, "_strnset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, "XX\0\xAA", 4) == 0, "_strnset_s overflow content mismatch\n");

    /* Count of zero - no change */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "Hello\0", 6);
    ret = p_strnset_s(buf, sizeof(buf), 'X', 0);
    ok(ret == 0, "_strnset_s(count=0) returned %d, expected 0\n", ret);
    ok(memcmp(buf, "Hello\0\xAA", 7) == 0, "_strnset_s(count=0) content mismatch\n");

    /* Empty string with count > 0 */
    memset(buf, 0xAA, sizeof(buf));
    buf[0] = '\0';
    ret = p_strnset_s(buf, sizeof(buf), 'X', 5);
    ok(ret == 0, "_strnset_s empty returned %d, expected 0\n", ret);
    ok(memcmp(buf, "\0\xAA", 2) == 0, "_strnset_s empty content mismatch\n");

    /* Exact buffer size */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "ABC\0", 4);
    ret = p_strnset_s(buf, 4, 'Z', 2);
    ok(ret == 0, "_strnset_s exact returned %d, expected 0\n", ret);
    ok(memcmp(buf, "ZZC\0\xAA", 5) == 0, "_strnset_s exact content mismatch\n");

    /* Fill with high byte */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "AB\0", 3);
    ret = p_strnset_s(buf, sizeof(buf), 0xFE, 1);
    ok(ret == 0, "_strnset_s high byte returned %d, expected 0\n", ret);
    ok(memcmp(buf, "\xFE" "B\0\xAA", 4) == 0, "_strnset_s high byte content mismatch\n");

    /* Value truncated to char (0x158 -> 'X' = 0x58) */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "AB\0", 3);
    ret = p_strnset_s(buf, sizeof(buf), 0x158, 2);
    ok(ret == 0, "_strnset_s truncated returned %d, expected 0\n", ret);
    ok(memcmp(buf, "XX\0\xAA", 4) == 0, "_strnset_s truncated content mismatch\n");

    /* Zero size */
    memset(buf, 0xAA, sizeof(buf));
    ret = p_strnset_s(buf, 0, 'X', 5);
    ok(ret == EINVAL, "_strnset_s(size=0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok((unsigned char)buf[0] == 0xAA, "_strnset_s(size=0): buf modified\n");

    /* No null terminator within size */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "ABCDEF", 6);
    ret = p_strnset_s(buf, 3, 'X', 5);
    ok(ret == EINVAL, "_strnset_s no-null returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(memcmp(buf, "\0XCDEF\xAA", 7) == 0, "_strnset_s no-null content mismatch\n");

    /* NULL string */
    ret = p_strnset_s(NULL, 10, 'X', 5);
    ok(ret == EINVAL, "_strnset_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* NULL string and zero size */
    ret = p_strnset_s(NULL, 0, 'X', 5);
    ok(ret == EINVAL, "_strnset_s(NULL, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* NULL string with zero count */
    ret = p_strnset_s(NULL, 10, 'X', 0);
    ok(ret == EINVAL, "_strnset_s(NULL, 10, X, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
}

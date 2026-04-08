/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _strset_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN__strset_s)(char* _Destination, size_t _DestinationSize, int _Value);

START_TEST(_strset_s)
{
    PFN__strset_s p_strset_s = (PFN__strset_s)load_function("_strset_s");
    char buf[64];
    errno_t ret;

    if (p_strset_s == NULL)
    {
        skip("Function _strset_s not available\n");
        return;
    }

    /* Basic: fill entire string */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "Hello\0", 6);
    ret = p_strset_s(buf, sizeof(buf), 'X');
    ok(ret == 0, "_strset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, "XXXXX\0\xAA", 7) == 0, "_strset_s basic content mismatch\n");

    /* Single character string */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "A\0", 2);
    ret = p_strset_s(buf, sizeof(buf), 'Z');
    ok(ret == 0, "_strset_s returned %d, expected 0\n", ret);
    ok(memcmp(buf, "Z\0\xAA", 3) == 0, "_strset_s single char content mismatch\n");

    /* Empty string - nothing to fill */
    memset(buf, 0xAA, sizeof(buf));
    buf[0] = '\0';
    ret = p_strset_s(buf, sizeof(buf), 'X');
    ok(ret == 0, "_strset_s empty returned %d, expected 0\n", ret);
    ok(memcmp(buf, "\0\xAA", 2) == 0, "_strset_s empty content mismatch\n");

    /* Exact size (string length + 1) */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "ABC\0", 4);
    ret = p_strset_s(buf, 4, 'Y');
    ok(ret == 0, "_strset_s exact size returned %d, expected 0\n", ret);
    ok(memcmp(buf, "YYY\0\xAA", 5) == 0, "_strset_s exact size content mismatch\n");

    /* Fill with null character (0) - sets all chars to null */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "Hello\0", 6);
    ret = p_strset_s(buf, sizeof(buf), '\0');
    ok(ret == 0, "_strset_s fill-null returned %d, expected 0\n", ret);
    ok(memcmp(buf, "\0\0\0\0\0\0\xAA", 7) == 0, "_strset_s fill-null content mismatch\n");

    /* Fill with high byte value */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "Hi\0", 3);
    ret = p_strset_s(buf, sizeof(buf), 0xFE);
    ok(ret == 0, "_strset_s high byte returned %d, expected 0\n", ret);
    ok(memcmp(buf, "\xFE\xFE\0\xAA", 4) == 0, "_strset_s high unicode content mismatch\n");

    /* Value truncated to char (0x158 -> 'X' = 0x58) */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "AB\0", 3);
    ret = p_strset_s(buf, sizeof(buf), 0x158);
    ok(ret == 0, "_strset_s truncated returned %d, expected 0\n", ret);
    ok(memcmp(buf, "XX\0\xAA", 4) == 0, "_strset_s truncated content mismatch\n");

    /* Zero size */
    memset(buf, 0xAA, sizeof(buf));
    ret = p_strset_s(buf, 0, 'X');
    ok(ret == EINVAL, "_strset_s(size=0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok((unsigned char)buf[0] == 0xAA, "_strset_s(size=0): buf modified\n");

    /* No null terminator within size */
    memset(buf, 0xAA, sizeof(buf));
    memcpy(buf, "ABCDEF", 6);
    ret = p_strset_s(buf, 3, 'X');
    ok(ret == EINVAL, "_strset_s no-null returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(memcmp(buf, "\0XCDEF\xAA", 7) == 0, "_strset_s truncated content mismatch\n");

    /* NULL string */
    ret = p_strset_s(NULL, 10, 'X');
    ok(ret == EINVAL, "_strset_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* NULL string and zero size */
    ret = p_strset_s(NULL, 0, 'X');
    ok(ret == EINVAL, "_strset_s(NULL, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
}

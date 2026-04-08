/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strncat_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_strncat_s)(char* _Destination, rsize_t _SizeInBytes, const char* _Source, rsize_t _MaxCount);

START_TEST(strncat_s)
{
    PFN_strncat_s p_strncat_s = (PFN_strncat_s)load_function("strncat_s");
    char dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_strncat_s, NULL);
        return;
    }

    if (p_strncat_s == NULL)
    {
        ok(FALSE, "strncat_s is not available\n");
        return;
    }

    /* Basic concatenation */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat_s(dst, sizeof(dst), " World", 10);
    ok(ret == 0, "strncat_s returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello World\0\xAA", 13) == 0, "strncat_s basic content mismatch\n");

    /* Count limits the copy */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat_s(dst, sizeof(dst), " World", 3);
    ok(ret == 0, "strncat_s returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello Wo\0\xAA", 10) == 0, "strncat_s limited content mismatch\n");

    /* Count = 0 appends nothing */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat_s(dst, sizeof(dst), " World", 0);
    ok(ret == 0, "strncat_s(count=0) returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strncat_s(count=0) content mismatch\n");

    /* Append to empty string */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = '\0';
    ret = p_strncat_s(dst, sizeof(dst), "Test", 10);
    ok(ret == 0, "strncat_s to-empty returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Test\0\xAA", 6) == 0, "strncat_s to-empty content mismatch\n");

    /* Append empty string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat_s(dst, sizeof(dst), "", 10);
    ok(ret == 0, "strncat_s empty-append returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strncat_s empty-append content mismatch\n");

    /* Exact fit: existing "Hi" (2 chars) + " X" (2 chars) + null = 5 in buffer of 5 */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strncat_s(dst, 5, " X", 10);
    ok(ret == 0, "strncat_s exact fit returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hi X\0\xAA", 6) == 0, "strncat_s exact fit content mismatch\n");

    /* Source ends before count, fits exactly */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "AB", 3);
    ret = p_strncat_s(dst, 5, "CD", 10);
    ok(ret == 0, "strncat_s src-ends returned %d, expected 0\n", ret);
    ok(memcmp(dst, "ABCD\0\xAA", 6) == 0, "strncat_s src-ends content mismatch\n");

    /* _TRUNCATE - truncates and null-terminates, returns STRUNCATE */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strncat_s(dst, 5, "XYZ!", _TRUNCATE);
    ok(ret == STRUNCATE, "strncat_s _TRUNCATE returned %d, expected STRUNCATE (%d)\n", ret, STRUNCATE);
    ok(memcmp(dst, "HiXY\0\xAA", 6) == 0, "strncat_s _TRUNCATE content mismatch\n");

    /* _TRUNCATE with sufficient buffer */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strncat_s(dst, sizeof(dst), "!", _TRUNCATE);
    ok(ret == 0, "strncat_s _TRUNCATE sufficient returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hi!\0\xAA", 5) == 0, "strncat_s _TRUNCATE sufficient content mismatch\n");

    /* _TRUNCATE with exactly enough room */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strncat_s(dst, 4, "X", _TRUNCATE);
    ok(ret == 0, "strncat_s _TRUNCATE exact returned %d, expected 0\n", ret);
    ok(memcmp(dst, "HiX\0\xAA", 5) == 0, "strncat_s _TRUNCATE exact content mismatch\n");

    /* _TRUNCATE with no room for any chars (only null fits) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strncat_s(dst, 3, "XYZ", _TRUNCATE);
    ok(ret == STRUNCATE, "strncat_s _TRUNCATE no-room returned %d, expected STRUNCATE (%d)\n", ret, STRUNCATE);
    ok(memcmp(dst, "Hi\0\xAA", 4) == 0, "strncat_s _TRUNCATE no-room content mismatch\n");

    /* Buffer too small - returns ERANGE, dest zeroed */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strncat_s(dst, 4, "XYZ", 10);
    ok(ret == ERANGE, "strncat_s small returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == '\0', "strncat_s small: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* NULL destination */
    ret = p_strncat_s(NULL, 10, "Hello", 5);
    ok(ret == EINVAL, "strncat_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Zero size destination */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncat_s(dst, 0, "Hello", 5);
    ok(ret == EINVAL, "strncat_s(dst, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok((unsigned char)dst[0] == 0xAA, "strncat_s(dst, 0): dst modified\n");

    /* NULL source - dest gets zeroed */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strncat_s(dst, sizeof(dst), NULL, 5);
    ok(ret == EINVAL, "strncat_s(NULL src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == '\0', "strncat_s NULL src: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* Destination not null-terminated within buffer */
    memset(dst, 'X', sizeof(dst));
    ret = p_strncat_s(dst, 4, "AB", 2);
    ok(ret == EINVAL, "strncat_s unterminated returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == '\0', "strncat_s unterminated: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);
}

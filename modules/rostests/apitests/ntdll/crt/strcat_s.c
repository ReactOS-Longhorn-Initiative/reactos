/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strcat_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_strcat_s)(char* _Destination, rsize_t _SizeInBytes, const char* _Source);

START_TEST(strcat_s)
{
    PFN_strcat_s p_strcat_s = (PFN_strcat_s)load_function("strcat_s");
    char dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_strcat_s, NULL);
        return;
    }

    if (p_strcat_s == NULL)
    {
        ok(FALSE, "strcat_s is not available\n");
        return;
    }

    /* Basic concatenation */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strcat_s(dst, sizeof(dst), " World");
    ok(ret == 0, "strcat_s returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello World\0\xAA", 13) == 0, "strcat_s basic content mismatch\n");

    /* Append to empty string */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = '\0';
    ret = p_strcat_s(dst, sizeof(dst), "Test");
    ok(ret == 0, "strcat_s to-empty returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Test\0\xAA", 6) == 0, "strcat_s to-empty content mismatch\n");

    /* Append empty string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strcat_s(dst, sizeof(dst), "");
    ok(ret == 0, "strcat_s empty-append returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strcat_s empty-append content mismatch\n");

    /* Exact fit: "Hi" (2) + " X" (2) + null = 5 in buffer of 5 */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strcat_s(dst, 5, " X");
    ok(ret == 0, "strcat_s exact fit returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hi X\0\xAA", 6) == 0, "strcat_s exact fit content mismatch\n");

    /* Buffer too small - returns ERANGE, dest zeroed */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strcat_s(dst, 4, "XYZ");
    ok(ret == ERANGE, "strcat_s small returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == '\0', "strcat_s small: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* Buffer size allows only null after existing string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hi", 3);
    ret = p_strcat_s(dst, 3, "X");
    ok(ret == ERANGE, "strcat_s no-room returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == '\0', "strcat_s no-room: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* NULL destination */
    ret = p_strcat_s(NULL, 10, "Hello");
    ok(ret == EINVAL, "strcat_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Zero size destination */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcat_s(dst, 0, "Hello");
    ok(ret == EINVAL, "strcat_s(dst, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok((unsigned char)dst[0] == 0xAA, "strcat_s(dst, 0): dst modified\n");

    /* NULL source - dest gets zeroed */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello", 6);
    ret = p_strcat_s(dst, sizeof(dst), NULL);
    ok(ret == EINVAL, "strcat_s(NULL src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == '\0', "strcat_s NULL src: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* Destination not null-terminated within buffer */
    memset(dst, 'X', sizeof(dst));
    ret = p_strcat_s(dst, 4, "AB");
    ok(ret == EINVAL, "strcat_s unterminated returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == '\0', "strcat_s unterminated: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* Special characters */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "A\x80\xFE", 4);
    ret = p_strcat_s(dst, sizeof(dst), "\t\n\xFF");
    ok(ret == 0, "strcat_s special returned %d, expected 0\n", ret);
    ok(memcmp(dst, "A\x80\xFE\t\n\xFF\0\xAA", 8) == 0, "strcat_s special content mismatch\n");

    /* Multiple concatenations */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = '\0';
    p_strcat_s(dst, sizeof(dst), "abc");
    p_strcat_s(dst, sizeof(dst), "def");
    p_strcat_s(dst, sizeof(dst), "ghi");
    ok(memcmp(dst, "abcdefghi\0\xAA", 11) == 0, "strcat_s multi content mismatch\n");
}

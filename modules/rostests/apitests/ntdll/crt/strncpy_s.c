/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strncpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_strncpy_s)(char* _Destination, rsize_t _SizeInBytes, const char* _Source, rsize_t _MaxCount);

START_TEST(strncpy_s)
{
    PFN_strncpy_s p_strncpy_s = (PFN_strncpy_s)load_function("strncpy_s");
    char dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_strncpy_s, NULL);
        return;
    }

    if (p_strncpy_s == NULL)
    {
        ok(FALSE, "strncpy_s is not available\n");
        return;
    }

    /* Basic copy - source fits, count >= source length */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, sizeof(dst), "Hello", 10);
    ok(ret == 0, "strncpy_s returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strncpy_s basic content mismatch\n");

    /* Count limits the copy */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, sizeof(dst), "Hello World", 5);
    ok(ret == 0, "strncpy_s returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strncpy_s truncated content mismatch\n");

    /* Count of zero - destination gets null terminated */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, sizeof(dst), "Hello", 0);
    ok(ret == 0, "strncpy_s(count=0) returned %d, expected 0\n", ret);
    ok(memcmp(dst, "\0\xAA", 2) == 0, "strncpy_s(count=0) content mismatch\n");

    /* Exact fit: buffer size = source length + 1 */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, 6, "Hello", 10);
    ok(ret == 0, "strncpy_s exact fit returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strncpy_s exact fit content mismatch\n");

    /* Empty source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, sizeof(dst), "", 10);
    ok(ret == 0, "strncpy_s empty returned %d, expected 0\n", ret);
    ok(memcmp(dst, "\0\xAA", 2) == 0, "strncpy_s empty content mismatch\n");

    /* _TRUNCATE - truncates and null-terminates, returns STRUNCATE */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, 4, "Hello", _TRUNCATE);
    ok(ret == STRUNCATE, "strncpy_s _TRUNCATE returned %d, expected STRUNCATE (%d)\n", ret, STRUNCATE);
    ok(memcmp(dst, "Hel\0\xAA", 5) == 0, "strncpy_s _TRUNCATE content mismatch\n");

    /* _TRUNCATE with sufficient buffer */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, sizeof(dst), "Hi", _TRUNCATE);
    ok(ret == 0, "strncpy_s _TRUNCATE sufficient returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hi\0\xAA", 4) == 0, "strncpy_s _TRUNCATE sufficient content mismatch\n");

    /* Buffer too small - returns ERANGE */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, 3, "Hello", 10);
    ok(ret == ERANGE, "strncpy_s small buffer returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == '\0', "strncpy_s small buffer: dst[0] = 0x%02x\n", (unsigned char)dst[0]);

    /* NULL destination */
    ret = p_strncpy_s(NULL, 10, "Hello", 5);
    ok(ret == EINVAL, "strncpy_s(NULL, ...) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Zero size destination */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, 0, "Hello", 5);
    ok(ret == EINVAL, "strncpy_s(dst, 0, ...) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok((unsigned char)dst[0] == 0xAA, "strncpy_s(dst, 0): dst modified\n");

    /* NULL source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strncpy_s(dst, sizeof(dst), NULL, 5);
    ok(ret == EINVAL, "strncpy_s(dst, n, NULL, 5) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == '\0', "strncpy_s NULL src: dst[0] = 0x%02x\n", (unsigned char)dst[0]);

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello\0", 6);
    ret = p_strncpy_s(dst, sizeof(dst), dst, 10);
    ok(ret == 0, "strncpy_s self returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strncpy_s self content mismatch\n");

    /* Overlapping: src at positive offset from dst */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello World\0", 12);
    ret = p_strncpy_s(dst, sizeof(dst), dst + 6, 10);
    ok(ret == 0, "strncpy_s fwd-overlap returned %d, expected 0\n", ret);
    ok(memcmp(dst, "World\0World\0\xAA", 13) == 0, "strncpy_s fwd-overlap content mismatch\n");

    /* Overlapping: dst at positive offset from src - rejected as overlapping */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello\0", 6);
    ret = p_strncpy_s(dst + 2, 10, dst, 10);
    ok(ret == ERANGE, "strncpy_s bwd-overlap returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(memcmp(dst, "He\0eHeHeHeHe\xAA\xAA", 14) == 0, "strncpy_s bwd-overlap content mismatch\n");
}

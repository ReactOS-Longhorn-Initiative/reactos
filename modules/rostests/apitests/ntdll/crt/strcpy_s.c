/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for strcpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_strcpy_s)(char* _Destination, rsize_t _SizeInBytes, const char* _Source);

START_TEST(strcpy_s)
{
    PFN_strcpy_s p_strcpy_s = (PFN_strcpy_s)load_function("strcpy_s");
    char dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_strcpy_s, NULL);
        return;
    }

    if (p_strcpy_s == NULL)
    {
        ok(FALSE, "strcpy_s is not available\n");
        return;
    }

    /* Basic copy with sufficient buffer */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy_s(dst, sizeof(dst), "Hello");
    ok(ret == 0, "strcpy_s returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strcpy_s basic content mismatch\n");

    /* Exact fit */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy_s(dst, 6, "Hello");
    ok(ret == 0, "strcpy_s exact fit returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strcpy_s exact fit content mismatch\n");

    /* Empty source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy_s(dst, sizeof(dst), "");
    ok(ret == 0, "strcpy_s empty returned %d, expected 0\n", ret);
    ok(memcmp(dst, "\0\xAA", 2) == 0, "strcpy_s empty content mismatch\n");

    /* Buffer too small - returns ERANGE, dst[0] set to null */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy_s(dst, 3, "Hello");
    ok(ret == ERANGE, "strcpy_s small buffer returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == '\0', "strcpy_s small buffer: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* Buffer size 1 with non-empty source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy_s(dst, 1, "A");
    ok(ret == ERANGE, "strcpy_s size=1 returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == '\0', "strcpy_s size=1: dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* Buffer size 1 with empty source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy_s(dst, 1, "");
    ok(ret == 0, "strcpy_s size=1 empty returned %d, expected 0\n", ret);
    ok(dst[0] == '\0', "strcpy_s size=1 empty: dst[0] = 0x%02x\n", (unsigned char)dst[0]);

    /* NULL destination */
    ret = p_strcpy_s(NULL, 10, "Hello");
    ok(ret == EINVAL, "strcpy_s(NULL, 10, src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Zero size */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy_s(dst, 0, "Hello");
    ok(ret == EINVAL, "strcpy_s(dst, 0, src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok((unsigned char)dst[0] == 0xAA, "strcpy_s(dst, 0): dst modified\n");

    /* NULL source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_strcpy_s(dst, sizeof(dst), NULL);
    ok(ret == EINVAL, "strcpy_s(dst, n, NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == '\0', "strcpy_s(dst, n, NULL): dst[0] = 0x%02x, expected 0\n", (unsigned char)dst[0]);

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello\0", 6);
    ret = p_strcpy_s(dst, sizeof(dst), dst);
    ok(ret == 0, "strcpy_s self returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\0\xAA", 7) == 0, "strcpy_s self content mismatch\n");

    /* Overlapping: src at positive offset from dst */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello World\0", 12);
    ret = p_strcpy_s(dst, sizeof(dst), dst + 6);
    ok(ret == 0, "strcpy_s fwd-overlap returned %d, expected 0\n", ret);
    ok(memcmp(dst, "World\0World\0\xAA", 13) == 0, "strcpy_s fwd-overlap content mismatch\n");

    /* Overlapping: dst at positive offset from src - rejected as overlapping */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello\0", 6);
    ret = p_strcpy_s(dst + 2, 10, dst);
    ok(ret == ERANGE, "strcpy_s bwd-overlap returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(memcmp(dst, "He\0eHeHeHeHe\xAA\xAA", 14) == 0, "strcpy_s bwd-overlap content mismatch\n");
}

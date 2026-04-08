/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for memcpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_memcpy_s)(void* _Dst, rsize_t _DstSize, const void* _Src, rsize_t _SrcSize);

START_TEST(memcpy_s)
{
    PFN_memcpy_s p_memcpy_s = (PFN_memcpy_s)load_function("memcpy_s");
    char dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_memcpy_s, NULL);
        return;
    }

    if (p_memcpy_s == NULL)
    {
        ok(FALSE, "memcpy_s is not available\n");
        return;
    }

    *ntdll_errno() = 0xdeadbeef;

    /* Basic copy */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, sizeof(dst), "Hello", 5);
    ok(ret == 0, "memcpy_s returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello\xAA", 6) == 0, "memcpy_s basic content mismatch\n");

    /* Copy single byte */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, sizeof(dst), "X", 1);
    ok(ret == 0, "memcpy_s returned %d, expected 0\n", ret);
    ok(memcmp(dst, "X\xAA", 2) == 0, "memcpy_s single byte content mismatch\n");

    /* Copy with exact destination size */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, 5, "ABCDE", 5);
    ok(ret == 0, "memcpy_s exact returned %d, expected 0\n", ret);
    ok(memcmp(dst, "ABCDE\xAA", 6) == 0, "memcpy_s exact content mismatch\n");

    /* Zero source size - no copy, returns 0 */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, sizeof(dst), "Hello", 0);
    ok(ret == 0, "memcpy_s(srcsize=0) returned %d, expected 0\n", ret);
    ok((unsigned char)dst[0] == 0xAA, "memcpy_s(srcsize=0): dst modified\n");

    /* Zero source size with NULL source - returns 0 */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, sizeof(dst), NULL, 0);
    ok(ret == 0, "memcpy_s(NULL, 0) returned %d, expected 0\n", ret);
    ok((unsigned char)dst[0] == 0xAA, "memcpy_s(NULL, 0): dst modified\n");

    /* Source size larger than destination - zeros dst, returns ERANGE */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, 3, "Hello", 5);
    ok(ret == ERANGE, "memcpy_s small dst returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(memcmp(dst, "\0\0\0\xAA", 4) == 0, "memcpy_s small dst content mismatch\n");

    /* NULL source with non-zero size - zeros dst, returns EINVAL */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, 7, NULL, 5);
    ok(ret == EINVAL, "memcpy_s(NULL src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(memcmp(dst, "\0\0\0\0\0\0\0\xAA", 7) == 0, "memcpy_s small dst content mismatch\n");

    /* NULL source and destination size too small - zeros dst, returns EINVAL (NULL checked first) */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, 3, NULL, 5);
    ok(ret == EINVAL, "memcpy_s(NULL src, small dst) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(memcmp(dst, "\0\0\0\xAA", 4) == 0, "memcpy_s(NULL src, small dst) content mismatch\n");

    /* NULL destination with zero source/dest size - returns 0 */
    ret = p_memcpy_s(NULL, 0, "Hello", 0);
    ok(ret == 0, "memcpy_s(NULL dst, 0) returned %d, expected 0\n", ret);

    /* NULL destination with zero source and non-zero dest size - returns 0 */
    ret = p_memcpy_s(NULL, 10, "Hello", 0);
    ok(ret == 0, "memcpy_s(NULL dst) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* NULL destination with non-zero source and zero dest size - returns EINVAL */
    ret = p_memcpy_s(NULL, 0, "Hello", 5);
    ok(ret == EINVAL, "memcpy_s(NULL dst) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* NULL destination with non-zero sizes - returns EINVAL */
    ret = p_memcpy_s(NULL, 10, "Hello", 5);
    ok(ret == EINVAL, "memcpy_s(NULL dst) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Copy with embedded nulls */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, sizeof(dst), "A\0B\0C", 5);
    ok(ret == 0, "memcpy_s embedded nulls returned %d, expected 0\n", ret);
    ok(memcmp(dst, "A\0B\0C\xAA", 6) == 0, "memcpy_s embedded nulls content mismatch\n");

    /* Copy high byte values */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_memcpy_s(dst, sizeof(dst), "\x80\xFE\xFF\x00\x01", 5);
    ok(ret == 0, "memcpy_s high bytes returned %d, expected 0\n", ret);
    ok(memcmp(dst, "\x80\xFE\xFF\x00\x01\xAA", 6) == 0, "memcpy_s high bytes content mismatch\n");

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello World\0", 12);
    ret = p_memcpy_s(dst, sizeof(dst), dst, 12);
    ok(ret == 0, "memcpy_s self returned %d, expected 0\n", ret);
    ok(memcmp(dst, "Hello World\0\xAA", 13) == 0, "memcpy_s self content mismatch\n");

    /* Overlapping: src at positive offset from dst (forward overlap) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello World\0", 12);
    ret = p_memcpy_s(dst, sizeof(dst), dst + 6, 5);
    ok(ret == 0, "memcpy_s fwd-overlap returned %d, expected 0\n", ret);
    ok(memcmp(dst, "World World\0\xAA", 13) == 0, "memcpy_s fwd-overlap content mismatch\n");

    /* Overlapping: dst at positive offset from src (backward overlap) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, "Hello\0", 6);
    ret = p_memcpy_s(dst + 2, sizeof(dst) - 2, dst, 5);
    ok(ret == 0, "memcpy_s bwd-overlap returned %d, expected 0\n", ret);
    ok(memcmp(dst, "HeHello\xAA", 8) == 0, "memcpy_s bwd-overlap content mismatch\n");

    /* Verify errno was not modified */
    ok_eq_errno(*ntdll_errno(), 0xdeadbeef);
}

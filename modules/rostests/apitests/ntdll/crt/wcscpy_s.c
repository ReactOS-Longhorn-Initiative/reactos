/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcscpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_wcscpy_s)(wchar_t* _Destination, rsize_t _SizeInWords, const wchar_t* _Source);

START_TEST(wcscpy_s)
{
    PFN_wcscpy_s p_wcscpy_s = (PFN_wcscpy_s)load_function("wcscpy_s");
    wchar_t dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_wcscpy_s, NULL);
        return;
    }

    if (p_wcscpy_s == NULL)
    {
        ok(FALSE, "wcscpy_s is not available\n");
        return;
    }

    /* Basic copy with sufficient buffer */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy_s(dst, sizeof(dst) / sizeof(wchar_t), L"Hello");
    ok(ret == 0, "wcscpy_s returned %d, expected 0\n", ret);
    ok((wcscmp(dst, L"Hello") == 0) && (dst[6] == (wchar_t)0xAAAA),
       "wcscpy_s basic content mismatch\n");

    /* Exact fit */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy_s(dst, 6, L"Hello");
    ok(ret == 0, "wcscpy_s exact fit returned %d, expected 0\n", ret);
    ok((wcscmp(dst, L"Hello") == 0) && (dst[6] == (wchar_t)0xAAAA),
       "wcscpy_s exact fit content mismatch\n");

    /* Empty source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy_s(dst, sizeof(dst) / sizeof(wchar_t), L"");
    ok(ret == 0, "wcscpy_s empty returned %d, expected 0\n", ret);
    ok((dst[0] == L'\0') && (dst[1] == (wchar_t)0xAAAA),
       "wcscpy_s empty content mismatch\n");

    /* Buffer too small */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy_s(dst, 3, L"Hello");
    ok(ret == ERANGE, "wcscpy_s small returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == L'\0', "wcscpy_s small: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);

    /* Buffer size 1 with non-empty source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy_s(dst, 1, L"A");
    ok(ret == ERANGE, "wcscpy_s size=1 returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == L'\0', "wcscpy_s size=1: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);

    /* Buffer size 1 with empty source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy_s(dst, 1, L"");
    ok(ret == 0, "wcscpy_s size=1 empty returned %d, expected 0\n", ret);
    ok(dst[0] == L'\0', "wcscpy_s size=1 empty: dst[0] = 0x%04x\n", (unsigned)dst[0]);

    /* NULL destination */
    ret = p_wcscpy_s(NULL, 10, L"Hello");
    ok(ret == EINVAL, "wcscpy_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Zero size */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy_s(dst, 0, L"Hello");
    ok(ret == EINVAL, "wcscpy_s(dst, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == (wchar_t)0xAAAA, "wcscpy_s(dst, 0): dst modified\n");

    /* NULL source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscpy_s(dst, sizeof(dst) / sizeof(wchar_t), NULL);
    ok(ret == EINVAL, "wcscpy_s(NULL src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == L'\0', "wcscpy_s NULL src: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcscpy_s(dst, sizeof(dst) / sizeof(wchar_t), dst);
    ok(ret == 0, "wcscpy_s self returned %d, expected 0\n", ret);
    ok((wcscmp(dst, L"Hello") == 0) && (dst[6] == (wchar_t)0xAAAA),
       "wcscpy_s self content mismatch\n");

    /* Overlapping: src at positive offset from dst */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello World\0", 12 * sizeof(wchar_t));
    ret = p_wcscpy_s(dst, sizeof(dst) / sizeof(wchar_t), dst + 6);
    ok(ret == 0, "wcscpy_s fwd-overlap returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"World") == 0, "wcscpy_s fwd-overlap result mismatch\n");
    ok(memcmp(dst + 6, L"World\0", 6 * sizeof(wchar_t)) == 0, "wcscpy_s fwd-overlap old data changed\n");

    /* Overlapping: dst at positive offset from src - rejected as overlapping */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcscpy_s(dst + 2, 10, dst);
    ok(ret == ERANGE, "wcscpy_s bwd-overlap returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(memcmp(dst, L"He\0eHeHeHeHe\xAAAA\xAAAA", 14 * sizeof(wchar_t)) == 0, "wcscpy_s bwd-overlap content mismatch\n");
}

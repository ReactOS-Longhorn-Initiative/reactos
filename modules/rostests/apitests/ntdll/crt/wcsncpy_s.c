/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcsncpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_wcsncpy_s)(wchar_t* _Destination, rsize_t _SizeInWords, const wchar_t* _Source, rsize_t _MaxCount);

START_TEST(wcsncpy_s)
{
    PFN_wcsncpy_s p_wcsncpy_s = (PFN_wcsncpy_s)load_function("wcsncpy_s");
    wchar_t dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_wcsncpy_s, NULL);
        return;
    }

    if (p_wcsncpy_s == NULL)
    {
        ok(FALSE, "wcsncpy_s is not available\n");
        return;
    }

    /* Basic copy - source fits, count >= source length */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, sizeof(dst) / sizeof(wchar_t), L"Hello", 10);
    ok(ret == 0, "wcsncpy_s returned %d, expected 0\n", ret);
    ok((wcscmp(dst, L"Hello") == 0) && (dst[6] == (wchar_t)0xAAAA),
       "wcsncpy_s basic content mismatch\n");

    /* Count limits the copy */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, sizeof(dst) / sizeof(wchar_t), L"Hello World", 5);
    ok(ret == 0, "wcsncpy_s returned %d, expected 0\n", ret);
    ok((wcscmp(dst, L"Hello") == 0) && (dst[6] == (wchar_t)0xAAAA),
       "wcsncpy_s truncated content mismatch\n");

    /* Count of zero */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, sizeof(dst) / sizeof(wchar_t), L"Hello", 0);
    ok(ret == 0, "wcsncpy_s(count=0) returned %d, expected 0\n", ret);
    ok((dst[0] == L'\0') && (dst[1] == (wchar_t)0xAAAA),
       "wcsncpy_s(count=0) content mismatch\n");

    /* Exact fit */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, 6, L"Hello", 10);
    ok(ret == 0, "wcsncpy_s exact fit returned %d, expected 0\n", ret);
    ok((wcscmp(dst, L"Hello") == 0) && (dst[6] == (wchar_t)0xAAAA),
       "wcsncpy_s exact fit content mismatch\n");

    /* Empty source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, sizeof(dst) / sizeof(wchar_t), L"", 10);
    ok(ret == 0, "wcsncpy_s empty returned %d, expected 0\n", ret);
    ok((dst[0] == L'\0') && (dst[1] == (wchar_t)0xAAAA),
       "wcsncpy_s empty content mismatch\n");

    /* _TRUNCATE - truncates and null-terminates, returns STRUNCATE */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, 4, L"Hello", _TRUNCATE);
    ok(ret == STRUNCATE, "wcsncpy_s _TRUNCATE returned %d, expected STRUNCATE (%d)\n", ret, STRUNCATE);
    ok((wcscmp(dst, L"Hel") == 0) && (dst[4] == (wchar_t)0xAAAA),
       "wcsncpy_s _TRUNCATE content mismatch\n");

    /* _TRUNCATE with sufficient buffer */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, sizeof(dst) / sizeof(wchar_t), L"Hi", _TRUNCATE);
    ok(ret == 0, "wcsncpy_s _TRUNCATE sufficient returned %d, expected 0\n", ret);
    ok((wcscmp(dst, L"Hi") == 0) && (dst[3] == (wchar_t)0xAAAA),
       "wcsncpy_s _TRUNCATE sufficient content mismatch\n");

    /* Buffer too small */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, 3, L"Hello", 10);
    ok(ret == ERANGE, "wcsncpy_s small returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == L'\0', "wcsncpy_s small: dst[0] = 0x%04x\n", (unsigned)dst[0]);

    /* NULL destination */
    ret = p_wcsncpy_s(NULL, 10, L"Hello", 5);
    ok(ret == EINVAL, "wcsncpy_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Zero size */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, 0, L"Hello", 5);
    ok(ret == EINVAL, "wcsncpy_s(dst, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == (wchar_t)0xAAAA, "wcsncpy_s(dst, 0): dst modified\n");

    /* NULL source */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncpy_s(dst, sizeof(dst) / sizeof(wchar_t), NULL, 5);
    ok(ret == EINVAL, "wcsncpy_s(NULL src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == L'\0', "wcsncpy_s NULL src: dst[0] = 0x%04x\n", (unsigned)dst[0]);

    /* Overlapping: dst == src (copy to self) */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsncpy_s(dst, sizeof(dst) / sizeof(wchar_t), dst, 10);
    ok(ret == 0, "wcsncpy_s self returned %d, expected 0\n", ret);
    ok((wcscmp(dst, L"Hello") == 0) && (dst[6] == (wchar_t)0xAAAA),
       "wcsncpy_s self content mismatch\n");

    /* Overlapping: src at positive offset from dst */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello World\0", 12 * sizeof(wchar_t));
    ret = p_wcsncpy_s(dst, sizeof(dst) / sizeof(wchar_t), dst + 6, 10);
    ok(ret == 0, "wcsncpy_s fwd-overlap returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"World") == 0, "wcsncpy_s fwd-overlap result mismatch\n");
    ok(memcmp(dst + 6, L"World\0", 6 * sizeof(wchar_t)) == 0, "wcsncpy_s fwd-overlap old data changed\n");

    /* Overlapping: dst at positive offset from src - rejected as overlapping */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello\0", 6 * sizeof(wchar_t));
    ret = p_wcsncpy_s(dst + 2, 10, dst, 10);
    ok(ret == ERANGE, "wcsncpy_s bwd-overlap returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(memcmp(dst, L"He\0eHeHeHeHe\xAAAA\xAAAA", 14 * sizeof(wchar_t)) == 0, "wcsncpy_s bwd-overlap content mismatch\n");
}

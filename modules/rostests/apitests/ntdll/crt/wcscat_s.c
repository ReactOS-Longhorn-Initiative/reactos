/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcscat_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_wcscat_s)(wchar_t* _Destination, rsize_t _SizeInWords, const wchar_t* _Source);

START_TEST(wcscat_s)
{
    PFN_wcscat_s p_wcscat_s = (PFN_wcscat_s)load_function("wcscat_s");
    wchar_t dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_wcscat_s, NULL);
        return;
    }

    if (p_wcscat_s == NULL)
    {
        ok(FALSE, "wcscat_s is not available\n");
        return;
    }

    /* Basic concatenation */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcscat_s(dst, sizeof(dst) / sizeof(wchar_t), L" World");
    ok(ret == 0, "wcscat_s returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hello World") == 0, "wcscat_s basic result mismatch\n");
    ok(dst[12] == (wchar_t)0xAAAA, "wcscat_s wrote past null\n");

    /* Append to empty string */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = L'\0';
    ret = p_wcscat_s(dst, sizeof(dst) / sizeof(wchar_t), L"Test");
    ok(ret == 0, "wcscat_s to-empty returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Test") == 0, "wcscat_s to-empty result mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcscat_s to-empty wrote past null\n");

    /* Append empty string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcscat_s(dst, sizeof(dst) / sizeof(wchar_t), L"");
    ok(ret == 0, "wcscat_s empty-append returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hello") == 0, "wcscat_s empty-append result mismatch\n");
    ok(dst[6] == (wchar_t)0xAAAA, "wcscat_s empty-append wrote past null\n");

    /* Exact fit: "Hi" (2) + " X" (2) + null = 5 in buffer of 5 */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcscat_s(dst, 5, L" X");
    ok(ret == 0, "wcscat_s exact fit returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hi X") == 0, "wcscat_s exact fit result mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcscat_s exact fit wrote past null\n");

    /* Buffer too small - returns ERANGE, dest zeroed */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcscat_s(dst, 4, L"XYZ");
    ok(ret == ERANGE, "wcscat_s small returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == L'\0', "wcscat_s small: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);

    /* NULL destination */
    ret = p_wcscat_s(NULL, 10, L"Hello");
    ok(ret == EINVAL, "wcscat_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Zero size destination */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcscat_s(dst, 0, L"Hello");
    ok(ret == EINVAL, "wcscat_s(dst, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == (wchar_t)0xAAAA, "wcscat_s(dst, 0): dst modified\n");

    /* NULL source - dest gets zeroed */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcscat_s(dst, sizeof(dst) / sizeof(wchar_t), NULL);
    ok(ret == EINVAL, "wcscat_s(NULL src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == L'\0', "wcscat_s NULL src: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);

    /* Destination not null-terminated within buffer */
    memset(dst, L'X', sizeof(dst));
    ret = p_wcscat_s(dst, 4, L"AB");
    ok(ret == EINVAL, "wcscat_s unterminated returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == L'\0', "wcscat_s unterminated: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);

    /* High Unicode values */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"\xFF00", 2 * sizeof(wchar_t));
    ret = p_wcscat_s(dst, sizeof(dst) / sizeof(wchar_t), L"\xFFFE");
    ok(ret == 0, "wcscat_s unicode returned %d, expected 0\n", ret);
    ok((dst[0] == (wchar_t)0xFF00) && (dst[1] == (wchar_t)0xFFFE) &&
       (dst[2] == L'\0') && (dst[3] == (wchar_t)0xAAAA),
       "wcscat_s unicode content mismatch\n");
}

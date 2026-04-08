/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for wcsncat_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN_wcsncat_s)(wchar_t* _Destination, rsize_t _SizeInWords, const wchar_t* _Source, rsize_t _MaxCount);

START_TEST(wcsncat_s)
{
    PFN_wcsncat_s p_wcsncat_s = (PFN_wcsncat_s)load_function("wcsncat_s");
    wchar_t dst[64];
    errno_t ret;

    if (GetNTVersion() < _WIN32_WINNT_WIN7)
    {
        ok_eq_pointer(p_wcsncat_s, NULL);
        return;
    }

    if (p_wcsncat_s == NULL)
    {
        ok(FALSE, "wcsncat_s is not available\n");
        return;
    }

    /* Basic concatenation */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, sizeof(dst) / sizeof(wchar_t), L" World", 10);
    ok(ret == 0, "wcsncat_s returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hello World") == 0, "wcsncat_s basic result mismatch\n");
    ok(dst[12] == (wchar_t)0xAAAA, "wcsncat_s wrote past null\n");

    /* Count limits the copy */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, sizeof(dst) / sizeof(wchar_t), L" World", 3);
    ok(ret == 0, "wcsncat_s returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hello Wo") == 0, "wcsncat_s limited result mismatch\n");
    ok(dst[9] == (wchar_t)0xAAAA, "wcsncat_s limited wrote past null\n");

    /* Count = 0 appends nothing */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, sizeof(dst) / sizeof(wchar_t), L" World", 0);
    ok(ret == 0, "wcsncat_s(count=0) returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hello") == 0, "wcsncat_s(count=0) result mismatch\n");
    ok(dst[6] == (wchar_t)0xAAAA, "wcsncat_s(count=0) wrote past null\n");

    /* Append to empty string */
    memset(dst, 0xAA, sizeof(dst));
    dst[0] = L'\0';
    ret = p_wcsncat_s(dst, sizeof(dst) / sizeof(wchar_t), L"Test", 10);
    ok(ret == 0, "wcsncat_s to-empty returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Test") == 0, "wcsncat_s to-empty result mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcsncat_s to-empty wrote past null\n");

    /* Append empty string */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, sizeof(dst) / sizeof(wchar_t), L"", 10);
    ok(ret == 0, "wcsncat_s empty-append returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hello") == 0, "wcsncat_s empty-append result mismatch\n");
    ok(dst[6] == (wchar_t)0xAAAA, "wcsncat_s empty-append wrote past null\n");

    /* Exact fit: "Hi" (2) + " X" (2) + null = 5 in buffer of 5 */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, 5, L" X", 10);
    ok(ret == 0, "wcsncat_s exact fit returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hi X") == 0, "wcsncat_s exact fit result mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcsncat_s exact fit wrote past null\n");

    /* _TRUNCATE - truncates and null-terminates, returns STRUNCATE */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, 5, L"XYZ!", _TRUNCATE);
    ok(ret == STRUNCATE, "wcsncat_s _TRUNCATE returned %d, expected STRUNCATE (%d)\n", ret, STRUNCATE);
    ok(wcscmp(dst, L"HiXY") == 0, "wcsncat_s _TRUNCATE result mismatch\n");
    ok(dst[5] == (wchar_t)0xAAAA, "wcsncat_s _TRUNCATE wrote past null\n");

    /* _TRUNCATE with sufficient buffer */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, sizeof(dst) / sizeof(wchar_t), L"!", _TRUNCATE);
    ok(ret == 0, "wcsncat_s _TRUNCATE sufficient returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"Hi!") == 0, "wcsncat_s _TRUNCATE sufficient result mismatch\n");
    ok(dst[4] == (wchar_t)0xAAAA, "wcsncat_s _TRUNCATE sufficient wrote past null\n");

    /* _TRUNCATE with exactly enough room */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, 4, L"X", _TRUNCATE);
    ok(ret == 0, "wcsncat_s _TRUNCATE exact returned %d, expected 0\n", ret);
    ok(wcscmp(dst, L"HiX") == 0, "wcsncat_s _TRUNCATE exact result mismatch\n");
    ok(dst[4] == (wchar_t)0xAAAA, "wcsncat_s _TRUNCATE exact wrote past null\n");

    /* _TRUNCATE with no room for any chars */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, 3, L"XYZ", _TRUNCATE);
    ok(ret == STRUNCATE, "wcsncat_s _TRUNCATE no-room returned %d, expected STRUNCATE (%d)\n", ret, STRUNCATE);
    ok(wcscmp(dst, L"Hi") == 0, "wcsncat_s _TRUNCATE no-room result mismatch\n");
    ok(dst[3] == (wchar_t)0xAAAA, "wcsncat_s _TRUNCATE no-room wrote past null\n");

    /* Buffer too small - returns ERANGE, dest zeroed */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hi", 3 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, 4, L"XYZ", 10);
    ok(ret == ERANGE, "wcsncat_s small returned %d, expected ERANGE (%d)\n", ret, ERANGE);
    ok(dst[0] == L'\0', "wcsncat_s small: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);

    /* NULL destination */
    ret = p_wcsncat_s(NULL, 10, L"Hello", 5);
    ok(ret == EINVAL, "wcsncat_s(NULL) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Zero size destination */
    memset(dst, 0xAA, sizeof(dst));
    ret = p_wcsncat_s(dst, 0, L"Hello", 5);
    ok(ret == EINVAL, "wcsncat_s(dst, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == (wchar_t)0xAAAA, "wcsncat_s(dst, 0): dst modified\n");

    /* NULL source - dest gets zeroed */
    memset(dst, 0xAA, sizeof(dst));
    memcpy(dst, L"Hello", 6 * sizeof(wchar_t));
    ret = p_wcsncat_s(dst, sizeof(dst) / sizeof(wchar_t), NULL, 5);
    ok(ret == EINVAL, "wcsncat_s(NULL src) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == L'\0', "wcsncat_s NULL src: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);

    /* Destination not null-terminated within buffer */
    memset(dst, L'X', sizeof(dst));
    ret = p_wcsncat_s(dst, 4, L"AB", 2);
    ok(ret == EINVAL, "wcsncat_s unterminated returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(dst[0] == L'\0', "wcsncat_s unterminated: dst[0] = 0x%04x, expected 0\n", (unsigned)dst[0]);
}

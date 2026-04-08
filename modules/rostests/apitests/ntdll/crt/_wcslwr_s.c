/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wcslwr_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>
#include <errno.h>
#include <windows.h>

typedef errno_t (__cdecl *PFN__wcslwr_s)(wchar_t* _String, size_t _Size);

static wchar_t expected_wcslwr(wchar_t ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch + ('a' - 'A');
    return ch;
}

START_TEST(_wcslwr_s)
{
    PFN__wcslwr_s p_wcslwr_s = (PFN__wcslwr_s)load_function("_wcslwr_s");
    wchar_t buffer[64];
    errno_t ret;

    if (p_wcslwr_s == NULL)
    {
        skip("Function _wcslwr_s not available\n");
        return;
    }

    /* Test basic string conversion */
    memset(buffer, 0xAA, sizeof(buffer));
    wcscpy(buffer, L"Hello, World! 123 \t\n\x80\xAB\xFE~`@#$%^&*()");
    ret = p_wcslwr_s(buffer, sizeof(buffer) / sizeof(wchar_t));
    ok(ret == 0, "_wcslwr_s returned %d, expected 0\n", ret);
    ok(wcscmp(buffer, L"hello, world! 123 \t\n\x80\xAB\xFE~`@#$%^&*()") == 0,
       "_wcslwr_s result mismatch\n");

    /* Test that the rest of the buffer is untouched */
    for (size_t i = wcslen(buffer) + 1; i < sizeof(buffer) / sizeof(wchar_t); i++)
    {
        ok(buffer[i] == 0xAAAAu,
           "_wcslwr_s: buffer[%zu] = 0x%02X, expected 0xAAAA\n",
           i, buffer[i]);
    }

    /* Test with exact size (string length + 1) */
    wcscpy(buffer, L"ABC");
    ret = p_wcslwr_s(buffer, 4);
    ok(ret == 0, "_wcslwr_s(L\"ABC\", 4) returned %d, expected 0\n", ret);
    ok(wcscmp(buffer, L"abc") == 0,
       "_wcslwr_s result mismatch, expected L\"abc\"\n");

    /* Test empty string */
    buffer[0] = L'\0';
    ret = p_wcslwr_s(buffer, 1);
    ok(ret == 0, "_wcslwr_s(L\"\", 1) returned %d, expected 0\n", ret);
    ok(buffer[0] == L'\0', "_wcslwr_s empty string modified\n");

    /* Test NULL string parameter */
    ret = p_wcslwr_s(NULL, 10);
    ok(ret == EINVAL, "_wcslwr_s(NULL, 10) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test zero size */
    wcscpy(buffer, L"Test");
    ret = p_wcslwr_s(buffer, 0);
    ok(ret == EINVAL, "_wcslwr_s(buffer, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test size too small (no null terminator within size) */
    wcscpy(buffer, L"ABCDEF");
    ret = p_wcslwr_s(buffer, 3);
    ok(ret == EINVAL, "_wcslwr_s(L\"ABCDEF\", 3) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(buffer[0] == L'\0',
       "_wcslwr_s with small size: buffer[0] = 0x%04x, expected 0x0000\n",
       (unsigned)buffer[0]);

    /* Test NULL string and zero size */
    ret = p_wcslwr_s(NULL, 0);
    ok(ret == EINVAL, "_wcslwr_s(NULL, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test all chars from 0x01 to 0xFFFF individually */
    for (unsigned int c = 1; c <= 0xFFFF; c++)
    {
        wchar_t expected_ch = expected_wcslwr((wchar_t)c);
        buffer[0] = (wchar_t)c;
        buffer[1] = L'\0';
        ret = p_wcslwr_s(buffer, 2);
        ok(ret == 0,
           "_wcslwr_s(0x%04x) returned %d, expected 0\n", c, ret);
        ok(buffer[0] == expected_ch,
           "_wcslwr_s(0x%04x): result = 0x%04x, expected 0x%04x\n",
           c, (unsigned)buffer[0], (unsigned)expected_ch);
    }
}

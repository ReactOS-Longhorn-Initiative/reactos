/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _strlwr_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>
#include <errno.h>
#include <windows.h>

typedef errno_t (__cdecl *PFN__strlwr_s)(char* _String, size_t _Size);

static char expected_strlwr(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch + ('a' - 'A');
    return ch;
}

START_TEST(_strlwr_s)
{
    PFN__strlwr_s p_strlwr_s = (PFN__strlwr_s)load_function("_strlwr_s");
    char buffer[64];
    errno_t ret;

    if (p_strlwr_s == NULL)
    {
        skip("Function _strlwr_s not available\n");
        return;
    }

    *ntdll_errno() = 0xdeadbeef;

    /* Test basic string conversion */
    memset(buffer, 0xAA, sizeof(buffer));
    strcpy(buffer, "Hello, World! 123 \t\n\x80\xAB\xFE~`@#$%^&*()");
    ret = p_strlwr_s(buffer, sizeof(buffer));
    ok(ret == 0, "_strlwr_s returned %d, expected 0\n", ret);
    ok(strcmp(buffer, "hello, world! 123 \t\n\x80\xAB\xFE~`@#$%^&*()") == 0,
       "_strlwr_s result = \"%s\"\n", buffer);

    /* Test that the rest of the buffer is untouched */
    for (size_t i = strlen(buffer) + 1; i < sizeof(buffer); i++)
    {
        ok((unsigned char)buffer[i] == 0xAAu,
           "_strlwr_s: buffer[%zu] = 0x%02X, expected 0xAA\n",
           i, (unsigned char)buffer[i]);
    }

    /* Test with exact size (string length + 1) */
    strcpy(buffer, "ABC");
    ret = p_strlwr_s(buffer, 4);
    ok(ret == 0, "_strlwr_s(\"ABC\", 4) returned %d, expected 0\n", ret);
    ok(strcmp(buffer, "abc") == 0,
       "_strlwr_s result = \"%s\", expected \"abc\"\n", buffer);

    /* Test empty string */
    buffer[0] = '\0';
    ret = p_strlwr_s(buffer, 1);
    ok(ret == 0, "_strlwr_s(\"\", 1) returned %d, expected 0\n", ret);
    ok(buffer[0] == '\0', "_strlwr_s empty string modified\n");

    /* Test NULL string parameter */
    ret = p_strlwr_s(NULL, 10);
    ok(ret == EINVAL, "_strlwr_s(NULL, 10) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test zero size */
    strcpy(buffer, "Test");
    ret = p_strlwr_s(buffer, 0);
    ok(ret == EINVAL, "_strlwr_s(buffer, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test size too small (no null terminator within size) */
    strcpy(buffer, "ABCDEF");
    ret = p_strlwr_s(buffer, 3);
    ok(ret == EINVAL, "_strlwr_s(\"ABCDEF\", 3) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(buffer[0] == '\0',
       "_strlwr_s with small size: buffer[0] = 0x%02x, expected 0x00\n",
       (unsigned char)buffer[0]);

    /* Test NULL string and zero size */
    ret = p_strlwr_s(NULL, 0);
    ok(ret == EINVAL, "_strlwr_s(NULL, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test all chars from 0x01 to 0xFF individually */
    for (unsigned int c = 1; c <= 0xFF; c++)
    {
        char expected_ch = expected_strlwr((char)c);
        buffer[0] = (char)c;
        buffer[1] = '\0';
        ret = p_strlwr_s(buffer, 2);
        ok(ret == 0,
           "_strlwr_s(0x%02x) returned %d, expected 0\n", c, ret);
        ok(buffer[0] == expected_ch,
           "_strlwr_s(0x%02x): result = 0x%02x, expected 0x%02x\n",
           c, (unsigned char)buffer[0], (unsigned char)expected_ch);
    }

    /* Test modification of errno */
    ok_eq_errno(*ntdll_errno(), 0xdeadbeef);
}

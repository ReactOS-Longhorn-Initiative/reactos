/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _strupr_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <string.h>
#include <errno.h>
#include <windows.h>

typedef errno_t (__cdecl *PFN__strupr_s)(char* _String, size_t _Size);

static char expected_strupr(char ch)
{
    if (ch >= 'a' && ch <= 'z')
        return ch + ('A' - 'a');
    return ch;
}

START_TEST(_strupr_s)
{
    PFN__strupr_s p_strupr_s = (PFN__strupr_s)load_function("_strupr_s");
    char buffer[64];
    errno_t ret;

    if (p_strupr_s == NULL)
    {
        skip("Function _strupr_s not available\n");
        return;
    }

    /* Test basic string conversion */
    memset(buffer, 0xAA, sizeof(buffer));
    strcpy(buffer, "Hello, World! 123 \t\n\x80\xAB\xFE~`@#$%^&*()");
    ret = p_strupr_s(buffer, sizeof(buffer));
    ok(ret == 0, "_strupr_s returned %d, expected 0\n", ret);
    ok(strcmp(buffer, "HELLO, WORLD! 123 \t\n\x80\xAB\xFE~`@#$%^&*()") == 0,
       "_strupr_s result = \"%s\"\n", buffer);

    /* Test that the rest of the buffer is untouched */
    for (size_t i = strlen(buffer) + 1; i < sizeof(buffer); i++)
    {
        ok((unsigned char)buffer[i] == 0xAAu,
           "_strupr_s: buffer[%zu] = 0x%02X, expected 0xAA\n",
           i, (unsigned char)buffer[i]);
    }

    /* Test with exact size (string length + 1) */
    strcpy(buffer, "abc");
    ret = p_strupr_s(buffer, 4);
    ok(ret == 0, "_strupr_s(\"abc\", 4) returned %d, expected 0\n", ret);
    ok(strcmp(buffer, "ABC") == 0,
       "_strupr_s result = \"%s\", expected \"ABC\"\n", buffer);

    /* Test empty string */
    buffer[0] = '\0';
    ret = p_strupr_s(buffer, 1);
    ok(ret == 0, "_strupr_s(\"\", 1) returned %d, expected 0\n", ret);
    ok(buffer[0] == '\0', "_strupr_s empty string modified\n");

    /* Test NULL string parameter */
    ret = p_strupr_s(NULL, 10);
    ok(ret == EINVAL, "_strupr_s(NULL, 10) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test zero size */
    strcpy(buffer, "Test");
    ret = p_strupr_s(buffer, 0);
    ok(ret == EINVAL, "_strupr_s(buffer, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test size too small (no null terminator within size) */
    strcpy(buffer, "abcdef");
    ret = p_strupr_s(buffer, 3);
    ok(ret == EINVAL, "_strupr_s(\"abcdef\", 3) returned %d, expected EINVAL (%d)\n", ret, EINVAL);
    ok(buffer[0] == '\0',
       "_strupr_s with small size: buffer[0] = 0x%02x, expected 0x00\n",
       (unsigned char)buffer[0]);

    /* Test NULL string and zero size */
    ret = p_strupr_s(NULL, 0);
    ok(ret == EINVAL, "_strupr_s(NULL, 0) returned %d, expected EINVAL (%d)\n", ret, EINVAL);

    /* Test all chars from 0x01 to 0xFF individually */
    for (unsigned int c = 1; c <= 0xFF; c++)
    {
        char expected_ch = expected_strupr((char)c);
        buffer[0] = (char)c;
        buffer[1] = '\0';
        ret = p_strupr_s(buffer, 2);
        ok(ret == 0,
           "_strupr_s(0x%02x) returned %d, expected 0\n", c, ret);
        ok(buffer[0] == expected_ch,
           "_strupr_s(0x%02x): result = 0x%02x, expected 0x%02x\n",
           c, (unsigned char)buffer[0], (unsigned char)expected_ch);
    }
}

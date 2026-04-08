/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _strlwr
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <windows.h>

typedef char* (__cdecl *PFN__strlwr)(char* _String);

static char expected_strlwr(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch + ('a' - 'A');
    return ch;
}

START_TEST(_strlwr)
{
    PFN__strlwr p_strlwr = (PFN__strlwr)load_function("_strlwr");
    char buffer[4];
    char *result;
    DWORD excpt_code;

    /* Test NULL parameter handling - _strlwr has no NULL check, expect STATUS_ACCESS_VIOLATION */
    excpt_code = 0;
    _SEH2_TRY
    {
        result = p_strlwr(NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        excpt_code = GetExceptionCode();
        result = (void*)(ULONG_PTR)0xdeaddead;
    }
    _SEH2_END;
    ok(excpt_code == STATUS_ACCESS_VIOLATION,
       "_strlwr(NULL) exception code = 0x%lx, expected STATUS_ACCESS_VIOLATIONx\n",
       excpt_code);

    /* Test a basic longer string with weird characters */
    char test_str[] = "Hello, World! 123 \t\n\x80\xAB\xFE~`@#$%^&*()";
    const char expected_str[] = "hello, world! 123 \t\n\x80\xAB\xFE~`@#$%^&*()";
    result = p_strlwr(test_str);
    ok(result == test_str, "_strlwr returned %p, expected %p\n", result, test_str);
    ok(strcmp(test_str, expected_str) == 0,
        "_strlwr result = \"%s\", expected \"%s\"\n", test_str, expected_str);

    /* Test all chars from 0x01 to 0xFF individually */
    for (unsigned int c = 1; c <= 0xFF; c++)
    {
        char expected_ch = expected_strlwr((char)c);
        buffer[0] = (char)c;
        buffer[1] = '\0';
        result = p_strlwr(buffer);
        ok(result == buffer,
           "_strlwr(0x%02x) returned %p, expected %p\n", c, result, buffer);
        ok(buffer[0] == expected_ch,
           "_strlwr(0x%02x): result = 0x%02x, expected 0x%02x\n",
           c, (unsigned char)buffer[0], (unsigned char)expected_ch);
    }
}

/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _scprintf
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <string.h>
#include <limits.h>
#include <Windows.h>

typedef int (__cdecl *PFN__scprintf)(size_t, const char*, ...);

#define ok_scprintf_(File, Line, ExpectedRet, Format, ...) \
    do { \
        int ret; \
        ret = p_scprintf(Format, ##__VA_ARGS__); \
        ok_(File, Line)(ret == ExpectedRet, "_scprintf returned %d, expected %d\n", ret, ExpectedRet); \
    } while (0)

#define ok_scprintf(ExpectedRet, Format, ...) \

START_TEST(_scprintf)
{
    HMODULE hdll = LoadLibraryA("ucrtbase.dll");
    PFN__scprintf p__scprintf = (PFN__scprintf)GetProcAddress(hdll, "_scprintf");
    if (!p__scprintf)
    {
        skip("_scprintf is not available\n");
        return;
    }

    /* Basic string */
    ok_scprintf(5, "Hello");

    /* Empty format string */
    ok_scprintf(0, "");

    /* NULL format string */
    ok_scprintf(-1, NULL);

    /* Argument passing - single string */
    ok_scprintf(5, "%s", "World");

    int ret;
    ret = p__scprintf(0, "%*d%*d", INT_MAX, 1, 1000, 1);
    ok_eq_int(ret, INT_MIN + 1000 - 1);

}

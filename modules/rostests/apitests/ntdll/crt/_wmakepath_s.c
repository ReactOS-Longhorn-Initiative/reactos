/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wmakepath_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN__wmakepath_s)(wchar_t*, size_t, const wchar_t*, const wchar_t*, const wchar_t*, const wchar_t*);

START_TEST(_wmakepath_s)
{
    PFN__wmakepath_s p_wmakepath_s = (PFN__wmakepath_s)load_function("_wmakepath_s");
    wchar_t buf[_MAX_PATH];
    errno_t ret;

    if (!p_wmakepath_s)
        return;

    /* All components */
    ret = p_wmakepath_s(buf, _MAX_PATH, L"C:", L"\\dir1\\dir2\\", L"file", L".ext");
    ok(ret == 0, "all ret = %d\n", ret);
    ok(wcscmp(buf, L"C:\\dir1\\dir2\\file.ext") == 0, "all = \"%ls\"\n", buf);

    /* Drive without colon */
    ret = p_wmakepath_s(buf, _MAX_PATH, L"C", L"\\dir\\", L"file", L".ext");
    ok(ret == 0, "nocolon ret = %d\n", ret);
    ok(wcscmp(buf, L"C:\\dir\\file.ext") == 0, "nocolon = \"%ls\"\n", buf);

    /* Dir without trailing separator */
    ret = p_wmakepath_s(buf, _MAX_PATH, L"C:", L"\\dir", L"file", L".ext");
    ok(ret == 0, "nosep ret = %d\n", ret);
    ok(wcscmp(buf, L"C:\\dir\\file.ext") == 0, "nosep = \"%ls\"\n", buf);

    /* Extension without dot */
    ret = p_wmakepath_s(buf, _MAX_PATH, L"C:", L"\\dir\\", L"file", L"ext");
    ok(ret == 0, "nodot ret = %d\n", ret);
    ok(wcscmp(buf, L"C:\\dir\\file.ext") == 0, "nodot = \"%ls\"\n", buf);

    /* All NULL components */
    ret = p_wmakepath_s(buf, _MAX_PATH, NULL, NULL, NULL, NULL);
    ok(ret == 0, "all null ret = %d\n", ret);
    ok(wcscmp(buf, L"") == 0, "all null = \"%ls\"\n", buf);

    /* NULL buffer */
    ret = p_wmakepath_s(NULL, _MAX_PATH, L"C:", L"\\", L"file", L".ext");
    ok(ret == EINVAL, "null buf ret = %d\n", ret);

    /* Zero buffer size */
    ret = p_wmakepath_s(buf, 0, L"C:", L"\\", L"file", L".ext");
    ok(ret == EINVAL, "zero size ret = %d\n", ret);

    /* Buffer too small */
    memset(buf, 0xAA, sizeof(buf));
    ret = p_wmakepath_s(buf, 5, L"C:", L"\\dir\\", L"file", L".ext");
    ok(ret == ERANGE, "small ret = %d\n", ret);
    ok(buf[0] == L'\0', "small buf[0] = 0x%04x\n", (unsigned)buf[0]);

    /* Buffer exact fit: "C:\dir\file.ext" = 15 chars + null = 16 */
    ret = p_wmakepath_s(buf, 16, L"C:", L"\\dir\\", L"file", L".ext");
    ok(ret == 0, "exact ret = %d\n", ret);
    ok(wcscmp(buf, L"C:\\dir\\file.ext") == 0, "exact = \"%ls\"\n", buf);

    /* Buffer one too small */
    memset(buf, 0xAA, sizeof(buf));
    ret = p_wmakepath_s(buf, 15, L"C:", L"\\dir\\", L"file", L".ext");
    ok(ret == ERANGE, "off by 1 ret = %d\n", ret);
    ok(buf[0] == L'\0', "off by 1 buf[0] = 0x%04x\n", (unsigned)buf[0]);

    /* NULL drive */
    ret = p_wmakepath_s(buf, _MAX_PATH, NULL, L"\\dir\\", L"file", L".ext");
    ok(ret == 0, "null drive ret = %d\n", ret);
    ok(wcscmp(buf, L"\\dir\\file.ext") == 0, "null drive = \"%ls\"\n", buf);

    /* NULL ext */
    ret = p_wmakepath_s(buf, _MAX_PATH, L"C:", L"\\dir\\", L"file", NULL);
    ok(ret == 0, "null ext ret = %d\n", ret);
    ok(wcscmp(buf, L"C:\\dir\\file") == 0, "null ext = \"%ls\"\n", buf);
}

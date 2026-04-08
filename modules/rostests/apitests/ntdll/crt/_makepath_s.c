/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _makepath_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN__makepath_s)(char*, size_t, const char*, const char*, const char*, const char*);

START_TEST(_makepath_s)
{
    PFN__makepath_s p_makepath_s = (PFN__makepath_s)load_function("_makepath_s");
    char buf[_MAX_PATH];
    errno_t ret;

    if (!p_makepath_s)
        return;

    /* All components */
    ret = p_makepath_s(buf, sizeof(buf), "C:", "\\dir1\\dir2\\", "file", ".ext");
    ok(ret == 0, "all ret = %d\n", ret);
    ok(strcmp(buf, "C:\\dir1\\dir2\\file.ext") == 0, "all = \"%s\"\n", buf);

    /* Drive without colon - colon is added */
    ret = p_makepath_s(buf, sizeof(buf), "C", "\\dir\\", "file", ".ext");
    ok(ret == 0, "drive nocolon ret = %d\n", ret);
    ok(strcmp(buf, "C:\\dir\\file.ext") == 0, "drive nocolon = \"%s\"\n", buf);

    /* Dir without trailing separator - backslash is added */
    ret = p_makepath_s(buf, sizeof(buf), "C:", "\\dir", "file", ".ext");
    ok(ret == 0, "dir nosep ret = %d\n", ret);
    ok(strcmp(buf, "C:\\dir\\file.ext") == 0, "dir nosep = \"%s\"\n", buf);

    /* Extension without dot - dot is added */
    ret = p_makepath_s(buf, sizeof(buf), "C:", "\\dir\\", "file", "ext");
    ok(ret == 0, "ext nodot ret = %d\n", ret);
    ok(strcmp(buf, "C:\\dir\\file.ext") == 0, "ext nodot = \"%s\"\n", buf);

    /* NULL drive */
    ret = p_makepath_s(buf, sizeof(buf), NULL, "\\dir\\", "file", ".ext");
    ok(ret == 0, "null drive ret = %d\n", ret);
    ok(strcmp(buf, "\\dir\\file.ext") == 0, "null drive = \"%s\"\n", buf);

    /* Empty drive */
    ret = p_makepath_s(buf, sizeof(buf), "", "\\dir\\", "file", ".ext");
    ok(ret == 0, "empty drive ret = %d\n", ret);
    ok(strcmp(buf, "\\dir\\file.ext") == 0, "empty drive = \"%s\"\n", buf);

    /* NULL dir */
    ret = p_makepath_s(buf, sizeof(buf), "C:", NULL, "file", ".ext");
    ok(ret == 0, "null dir ret = %d\n", ret);
    ok(strcmp(buf, "C:file.ext") == 0, "null dir = \"%s\"\n", buf);

    /* NULL filename */
    ret = p_makepath_s(buf, sizeof(buf), "C:", "\\dir\\", NULL, ".ext");
    ok(ret == 0, "null fname ret = %d\n", ret);
    ok(strcmp(buf, "C:\\dir\\.ext") == 0, "null fname = \"%s\"\n", buf);

    /* NULL extension */
    ret = p_makepath_s(buf, sizeof(buf), "C:", "\\dir\\", "file", NULL);
    ok(ret == 0, "null ext ret = %d\n", ret);
    ok(strcmp(buf, "C:\\dir\\file") == 0, "null ext = \"%s\"\n", buf);

    /* All NULL components */
    ret = p_makepath_s(buf, sizeof(buf), NULL, NULL, NULL, NULL);
    ok(ret == 0, "all null ret = %d\n", ret);
    ok(strcmp(buf, "") == 0, "all null = \"%s\"\n", buf);

    /* NULL buffer */
    ret = p_makepath_s(NULL, _MAX_PATH, "C:", "\\", "file", ".ext");
    ok(ret == EINVAL, "null buf ret = %d\n", ret);

    /* Zero buffer size */
    ret = p_makepath_s(buf, 0, "C:", "\\", "file", ".ext");
    ok(ret == EINVAL, "zero size ret = %d\n", ret);

    /* Buffer too small */
    memset(buf, 0xAA, sizeof(buf));
    ret = p_makepath_s(buf, 5, "C:", "\\dir\\", "file", ".ext");
    ok(ret == ERANGE, "small buf ret = %d\n", ret);
    ok(buf[0] == '\0', "small buf[0] = 0x%02x\n", (unsigned char)buf[0]);

    /* Buffer exact fit: "C:\dir\file.ext" = 15 chars + null = 16 */
    ret = p_makepath_s(buf, 16, "C:", "\\dir\\", "file", ".ext");
    ok(ret == 0, "exact ret = %d\n", ret);
    ok(strcmp(buf, "C:\\dir\\file.ext") == 0, "exact = \"%s\"\n", buf);

    /* Buffer one byte too small */
    memset(buf, 0xAA, sizeof(buf));
    ret = p_makepath_s(buf, 15, "C:", "\\dir\\", "file", ".ext");
    ok(ret == ERANGE, "off by 1 ret = %d\n", ret);
    ok(buf[0] == '\0', "off by 1 buf[0] = 0x%02x\n", (unsigned char)buf[0]);

    /* Dir ending with forward slash - no extra backslash added */
    ret = p_makepath_s(buf, sizeof(buf), "C:", "/dir/", "file", ".ext");
    ok(ret == 0, "fwdslash ret = %d\n", ret);
    ok(strcmp(buf, "C:/dir/file.ext") == 0, "fwdslash = \"%s\"\n", buf);

    /* Dir without trailing slash gets backslash appended */
    ret = p_makepath_s(buf, sizeof(buf), "C:", "/dir", "file", ".ext");
    ok(ret == 0, "fwddir ret = %d\n", ret);
    ok(strcmp(buf, "C:/dir\\file.ext") == 0, "fwddir = \"%s\"\n", buf);
}

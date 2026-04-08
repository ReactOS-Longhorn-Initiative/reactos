/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _splitpath_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN__splitpath_s)(const char*, char*, size_t, char*, size_t, char*, size_t, char*, size_t);

START_TEST(_splitpath_s)
{
    PFN__splitpath_s p_splitpath_s = (PFN__splitpath_s)load_function("_splitpath_s");
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    errno_t ret;

    if (!p_splitpath_s)
        return;

    /* Standard path with all components */
    ret = p_splitpath_s("C:\\dir1\\dir2\\file.ext", drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "ret = %d\n", ret);
    ok(strcmp(drive, "C:") == 0, "drive = \"%s\"\n", drive);
    ok(strcmp(dir, "\\dir1\\dir2\\") == 0, "dir = \"%s\"\n", dir);
    ok(strcmp(fname, "file") == 0, "fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".ext") == 0, "ext = \"%s\"\n", ext);

    /* NULL output pointers with zero counts */
    ret = p_splitpath_s("C:\\dir\\file.ext", NULL, 0, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == 0, "null outputs ret = %d\n", ret);

    /* Empty string */
    ret = p_splitpath_s("", drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "empty ret = %d\n", ret);
    ok(strcmp(drive, "") == 0, "empty drive = \"%s\"\n", drive);
    ok(strcmp(dir, "") == 0, "empty dir = \"%s\"\n", dir);
    ok(strcmp(fname, "") == 0, "empty fname = \"%s\"\n", fname);
    ok(strcmp(ext, "") == 0, "empty ext = \"%s\"\n", ext);

    /* NULL path should return EINVAL */
    memset(drive, 0xAA, sizeof(drive));
    ret = p_splitpath_s(NULL, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == EINVAL, "null path ret = %d\n", ret);
    ok(drive[0] == '\0', "null path drive[0] = 0x%02x\n", (unsigned char)drive[0]);

    /* Non-NULL drive with zero count should return EINVAL */
    ret = p_splitpath_s("C:\\file.ext", drive, 0, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == EINVAL, "drive zero count ret = %d\n", ret);

    /* NULL drive with non-zero count should return EINVAL */
    ret = p_splitpath_s("C:\\file.ext", NULL, _MAX_DRIVE, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == EINVAL, "null drive nonzero count ret = %d\n", ret);

    /* Drive buffer too small */
    memset(drive, 0xAA, sizeof(drive));
    ret = p_splitpath_s("C:\\file.ext", drive, 1, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == ERANGE, "drive too small ret = %d\n", ret);
    ok(drive[0] == '\0', "drive too small drive[0] = 0x%02x\n", (unsigned char)drive[0]);

    /* Ext buffer too small */
    memset(ext, 0xAA, sizeof(ext));
    ret = p_splitpath_s("C:\\dir\\file.ext", NULL, 0, NULL, 0, NULL, 0, ext, 1);
    ok(ret == ERANGE, "ext too small ret = %d\n", ret);
    ok(ext[0] == '\0', "ext too small ext[0] = 0x%02x\n", (unsigned char)ext[0]);

    /* Drive buffer exact fit */
    ret = p_splitpath_s("C:\\file.ext", drive, 3, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == 0, "drive exact ret = %d\n", ret);
    ok(strcmp(drive, "C:") == 0, "drive exact = \"%s\"\n", drive);

    /* Multiple dots */
    ret = p_splitpath_s("file.name.ext", NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "multidot ret = %d\n", ret);
    ok(strcmp(fname, "file.name") == 0, "multidot fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".ext") == 0, "multidot ext = \"%s\"\n", ext);

    /* Dot file - split at the last (only) dot */
    ret = p_splitpath_s(".gitignore", NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "dotfile ret = %d\n", ret);
    ok(strcmp(fname, "") == 0, "dotfile fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".gitignore") == 0, "dotfile ext = \"%s\"\n", ext);

    /* Trailing backslash */
    ret = p_splitpath_s("C:\\dir\\", NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "trailing ret = %d\n", ret);
    ok(strcmp(fname, "") == 0, "trailing fname = \"%s\"\n", fname);
    ok(strcmp(ext, "") == 0, "trailing ext = \"%s\"\n", ext);
}

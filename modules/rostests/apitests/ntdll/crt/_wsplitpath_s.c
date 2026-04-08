/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _wsplitpath_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef errno_t (__cdecl *PFN__wsplitpath_s)(const wchar_t*, wchar_t*, size_t, wchar_t*, size_t, wchar_t*, size_t, wchar_t*, size_t);

START_TEST(_wsplitpath_s)
{
    PFN__wsplitpath_s p_wsplitpath_s = (PFN__wsplitpath_s)load_function("_wsplitpath_s");
    wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    errno_t ret;

    if (!p_wsplitpath_s)
        return;

    /* Standard path with all components */
    ret = p_wsplitpath_s(L"C:\\dir1\\dir2\\file.ext", drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "ret = %d\n", ret);
    ok(wcscmp(drive, L"C:") == 0, "drive = \"%ls\"\n", drive);
    ok(wcscmp(dir, L"\\dir1\\dir2\\") == 0, "dir = \"%ls\"\n", dir);
    ok(wcscmp(fname, L"file") == 0, "fname = \"%ls\"\n", fname);
    ok(wcscmp(ext, L".ext") == 0, "ext = \"%ls\"\n", ext);

    /* NULL output pointers with zero counts */
    ret = p_wsplitpath_s(L"C:\\dir\\file.ext", NULL, 0, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == 0, "null outputs ret = %d\n", ret);

    /* Empty string */
    ret = p_wsplitpath_s(L"", drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "empty ret = %d\n", ret);
    ok(wcscmp(drive, L"") == 0, "empty drive = \"%ls\"\n", drive);
    ok(wcscmp(dir, L"") == 0, "empty dir = \"%ls\"\n", dir);
    ok(wcscmp(fname, L"") == 0, "empty fname = \"%ls\"\n", fname);
    ok(wcscmp(ext, L"") == 0, "empty ext = \"%ls\"\n", ext);

    /* NULL path should return EINVAL */
    memset(drive, 0xAA, sizeof(drive));
    ret = p_wsplitpath_s(NULL, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == EINVAL, "null path ret = %d\n", ret);
    ok(drive[0] == L'\0', "null path drive[0] = 0x%04x\n", (unsigned)drive[0]);

    /* Non-NULL drive with zero count should return EINVAL */
    ret = p_wsplitpath_s(L"C:\\file.ext", drive, 0, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == EINVAL, "drive zero count ret = %d\n", ret);

    /* NULL drive with non-zero count should return EINVAL */
    ret = p_wsplitpath_s(L"C:\\file.ext", NULL, _MAX_DRIVE, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == EINVAL, "null drive nonzero count ret = %d\n", ret);

    /* Drive buffer too small */
    memset(drive, 0xAA, sizeof(drive));
    ret = p_wsplitpath_s(L"C:\\file.ext", drive, 1, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == ERANGE, "drive too small ret = %d\n", ret);
    ok(drive[0] == L'\0', "drive too small drive[0] = 0x%04x\n", (unsigned)drive[0]);

    /* Drive exact fit */
    ret = p_wsplitpath_s(L"C:\\file.ext", drive, 3, NULL, 0, NULL, 0, NULL, 0);
    ok(ret == 0, "drive exact ret = %d\n", ret);
    ok(wcscmp(drive, L"C:") == 0, "drive exact = \"%ls\"\n", drive);

    /* Multiple dots */
    ret = p_wsplitpath_s(L"file.name.ext", NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "multidot ret = %d\n", ret);
    ok(wcscmp(fname, L"file.name") == 0, "multidot fname = \"%ls\"\n", fname);
    ok(wcscmp(ext, L".ext") == 0, "multidot ext = \"%ls\"\n", ext);

    /* Dot file - split at the last (only) dot */
    ret = p_wsplitpath_s(L".gitignore", NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "dotfile ret = %d\n", ret);
    ok(wcscmp(fname, L"") == 0, "dotfile fname = \"%ls\"\n", fname);
    ok(wcscmp(ext, L".gitignore") == 0, "dotfile ext = \"%ls\"\n", ext);

    /* Forward slashes */
    ret = p_wsplitpath_s(L"C:/dir/file.ext", drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
    ok(ret == 0, "fwd ret = %d\n", ret);
    ok(wcscmp(drive, L"C:") == 0, "fwd drive = \"%ls\"\n", drive);
    ok(wcscmp(dir, L"/dir/") == 0, "fwd dir = \"%ls\"\n", dir);
    ok(wcscmp(fname, L"file") == 0, "fwd fname = \"%ls\"\n", fname);
    ok(wcscmp(ext, L".ext") == 0, "fwd ext = \"%ls\"\n", ext);
}

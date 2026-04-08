/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _splitpath
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <string.h>

typedef void (__cdecl *PFN__splitpath)(const char*, char*, char*, char*, char*);

START_TEST(_splitpath)
{
    PFN__splitpath p_splitpath = (PFN__splitpath)load_function("_splitpath");
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

    if (!p_splitpath)
        return;

    /* Standard path with all components */
    p_splitpath("C:\\dir1\\dir2\\file.ext", drive, dir, fname, ext);
    ok(strcmp(drive, "C:") == 0, "drive = \"%s\"\n", drive);
    ok(strcmp(dir, "\\dir1\\dir2\\") == 0, "dir = \"%s\"\n", dir);
    ok(strcmp(fname, "file") == 0, "fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".ext") == 0, "ext = \"%s\"\n", ext);

    /* Path with forward slashes */
    p_splitpath("C:/dir1/dir2/file.ext", drive, dir, fname, ext);
    ok(strcmp(drive, "C:") == 0, "fwd drive = \"%s\"\n", drive);
    ok(strcmp(dir, "/dir1/dir2/") == 0, "fwd dir = \"%s\"\n", dir);
    ok(strcmp(fname, "file") == 0, "fwd fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".ext") == 0, "fwd ext = \"%s\"\n", ext);

    /* No drive */
    p_splitpath("\\dir\\file.txt", drive, dir, fname, ext);
    ok(strcmp(drive, "") == 0, "nodrive drive = \"%s\"\n", drive);
    ok(strcmp(dir, "\\dir\\") == 0, "nodrive dir = \"%s\"\n", dir);
    ok(strcmp(fname, "file") == 0, "nodrive fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".txt") == 0, "nodrive ext = \"%s\"\n", ext);

    /* No directory */
    p_splitpath("C:file.txt", drive, dir, fname, ext);
    ok(strcmp(drive, "C:") == 0, "nodir drive = \"%s\"\n", drive);
    ok(strcmp(dir, "") == 0, "nodir dir = \"%s\"\n", dir);
    ok(strcmp(fname, "file") == 0, "nodir fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".txt") == 0, "nodir ext = \"%s\"\n", ext);

    /* No extension */
    p_splitpath("C:\\dir\\file", drive, dir, fname, ext);
    ok(strcmp(drive, "C:") == 0, "noext drive = \"%s\"\n", drive);
    ok(strcmp(dir, "\\dir\\") == 0, "noext dir = \"%s\"\n", dir);
    ok(strcmp(fname, "file") == 0, "noext fname = \"%s\"\n", fname);
    ok(strcmp(ext, "") == 0, "noext ext = \"%s\"\n", ext);

    /* Just filename */
    p_splitpath("file.txt", drive, dir, fname, ext);
    ok(strcmp(drive, "") == 0, "justfile drive = \"%s\"\n", drive);
    ok(strcmp(dir, "") == 0, "justfile dir = \"%s\"\n", dir);
    ok(strcmp(fname, "file") == 0, "justfile fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".txt") == 0, "justfile ext = \"%s\"\n", ext);

    /* Empty string */
    p_splitpath("", drive, dir, fname, ext);
    ok(strcmp(drive, "") == 0, "empty drive = \"%s\"\n", drive);
    ok(strcmp(dir, "") == 0, "empty dir = \"%s\"\n", dir);
    ok(strcmp(fname, "") == 0, "empty fname = \"%s\"\n", fname);
    ok(strcmp(ext, "") == 0, "empty ext = \"%s\"\n", ext);

    /* Multiple dots - last dot separates extension */
    p_splitpath("C:\\dir\\file.name.ext", drive, dir, fname, ext);
    ok(strcmp(fname, "file.name") == 0, "multidot fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".ext") == 0, "multidot ext = \"%s\"\n", ext);

    /* Trailing backslash - no filename */
    p_splitpath("C:\\dir\\", drive, dir, fname, ext);
    ok(strcmp(dir, "\\dir\\") == 0, "trailing dir = \"%s\"\n", dir);
    ok(strcmp(fname, "") == 0, "trailing fname = \"%s\"\n", fname);

    /* NULL output pointers should not crash */
    p_splitpath("C:\\dir\\file.ext", NULL, NULL, NULL, NULL);

    /* Dot file - split at the last (only) dot */
    p_splitpath("C:\\dir\\.gitignore", drive, dir, fname, ext);
    ok(strcmp(fname, "") == 0, "dotfile fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".gitignore") == 0, "dotfile ext = \"%s\"\n", ext);

    /* Just a drive */
    p_splitpath("C:", drive, dir, fname, ext);
    ok(strcmp(drive, "C:") == 0, "justdrive drive = \"%s\"\n", drive);
    ok(strcmp(fname, "") == 0, "justdrive fname = \"%s\"\n", fname);

    /* Root path */
    p_splitpath("C:\\", drive, dir, fname, ext);
    ok(strcmp(dir, "\\") == 0, "root dir = \"%s\"\n", dir);

    /* Leading dot only */
    p_splitpath(".ext", drive, dir, fname, ext);
    ok(strcmp(fname, "") == 0, "justext fname = \"%s\"\n", fname);
    ok(strcmp(ext, ".ext") == 0, "justext ext = \"%s\"\n", ext);
}

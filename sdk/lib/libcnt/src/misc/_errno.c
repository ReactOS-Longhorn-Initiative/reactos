/*
 * PROJECT:     ReactOS NT User Mode Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Fallback implementation of _errno
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <errno.h>

static int global_errno = 0;

int* __cdecl _errno(void)
{
    /* Use a global dummy */
    return &global_errno;
}

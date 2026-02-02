/*
 * PROJECT:     ReactOS CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _local_unwind for AMD64
 * COPYRIGHT:   Copyright 2018-2021 Timo Kreuzer <timo.kreuzer@reactos.org>
*/

#include <windef.h>

void __cdecl _local_unwind(void* frame, void* target)
{
    RtlUnwind(frame, target, NULL, 0);
}

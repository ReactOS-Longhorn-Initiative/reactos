/*
 * PROJECT:     ReactOS NT User Mode Library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of _errno for user mode
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ntdll.h>

int* __cdecl _errno(void)
{
    /* In user mode errno is stored in TLS slot 16 in the TEB */
    return (int*)&NtCurrentTeb()->TlsSlots[16];
}

/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Multibyte capable version of toupper
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <ctype.h>
#include "../ntrtl.h"

_Check_return_
int
__cdecl
toupper(_In_ int const _C)
{
    PUCHAR ptr;
    WCHAR wc;
    CHAR chrUpper[2];
    ULONG mbSize;

    ptr = (PUCHAR)&_C;
    wc = RtlAnsiCharToUnicodeChar(&ptr);
    
    if (RtlUpcaseUnicodeToMultiByteN(chrUpper, sizeof(chrUpper), &mbSize, &wc, sizeof(wc)) < 0)
        return _C;

    if (mbSize == 2)
        return (UCHAR)chrUpper[1] + ((UCHAR)chrUpper[0] << 8);
    else
        return (UCHAR)chrUpper[0];
}

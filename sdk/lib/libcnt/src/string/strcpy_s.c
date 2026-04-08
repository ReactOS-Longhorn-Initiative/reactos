/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of strcpy_s
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <string.h>
#include <errno.h>

void __cdecl _invalid_parameter_noinfo(void);

_Check_return_wat_
errno_t
__cdecl
strcpy_s(
    _Out_writes_z_(SizeInBytes) char* const Destination,
    _In_ rsize_t const SizeInBytes,
    _In_z_ const char* const Source)
{
    if ((Destination == NULL) || (SizeInBytes == 0))
    {
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    if (Source == NULL)
    {
        Destination[0] = '\0';
        _invalid_parameter_noinfo();
        return EINVAL;
    }

    char* dst = Destination;
    const char* src = Source;
    rsize_t remaining = SizeInBytes;

    while (remaining--)
    {
        if ((*dst++ = *src++) == '\0')
            return 0;
    }

    /* Buffer too small - null-terminate and return error */
    Destination[0] = '\0';
    _invalid_parameter_noinfo();
    return ERANGE;
}

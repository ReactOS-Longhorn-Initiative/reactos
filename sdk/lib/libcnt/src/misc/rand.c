/*
 * PROJECT:     ReactOS NT CRT library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of rand and srand
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
*/

#include <stdlib.h>

static unsigned int s_rand_seed = 1; /* Default seed value */

_Check_return_
int
__cdecl
rand(void)
{
    /* Same LCG as in UCRT. */
    s_rand_seed = s_rand_seed * 214013 + 2531011;
    return (s_rand_seed >> 16) & RAND_MAX;
}

void
__cdecl
srand(_In_ unsigned int _Seed)
{
    s_rand_seed = _Seed;
}

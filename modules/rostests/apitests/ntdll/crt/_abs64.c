/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _abs64
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#include <stdlib.h>
#include <limits.h>

typedef __int64 (__cdecl *PFN__abs64)(__int64 _Number);

// Note: _abs64 is not exported by ntdll, tests are based on how the function should behave like.
START_TEST(_abs64)
{
    PFN__abs64 p_abs64 = (PFN__abs64)load_function("_abs64");
    if (!p_abs64)
    {
        ok(0, "_abs64 not found, skipping test\n");
        return;
    }

    /* Test zero */
    ok_eq_int64(p_abs64(0), 0);

    /* Test positive values */
    ok_eq_int64(p_abs64(1), 1);
    ok_eq_int64(p_abs64(42), 42);
    ok_eq_int64(p_abs64(0x7FFFFFFFFFFFFFFF), 0x7FFFFFFFFFFFFFFF);

    /* Test negative values */
    ok_eq_int64(p_abs64(-1), 1);
    ok_eq_int64(p_abs64(-42), 42);
    ok_eq_int64(p_abs64(-0x7FFFFFFFFFFFFFFF), 0x7FFFFFFFFFFFFFFF);

    /* Test values that cross the int range */
    ok_eq_int64(p_abs64(((__int64)INT_MAX) + 1), ((__int64)INT_MAX) + 1);
    ok_eq_int64(p_abs64(-((__int64)INT_MAX) - 2), ((__int64)INT_MAX) + 2);

    /* Test INT64_MIN - undefined behavior, reproduce ntdll's abs behavior */
    ok_eq_int64(p_abs64((-0x7FFFFFFFFFFFFFFF) - 1), (-0x7FFFFFFFFFFFFFFF) - 1);
}

/*
 * PROJECT:     NT C Runtime tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for _errno
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "test.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct _TEB
{
#ifdef _WIN64
    UCHAR Reserved1[0x1480];
#else
    UCHAR Reserved1[0xE10];
#endif
    void* TlsSlots[64];
};

typedef int* (*PFN_errno)(void);
typedef double (*PFN_sqrt)(double);

START_TEST(_errno)
{
    PFN_errno p__errno = load_function("_errno");
    PFN_sqrt psqrt = load_function("sqrt");
    struct _TEB* pteb = NtCurrentTeb();
    int* pErrno;

    if (p__errno != NULL)
    {
        /* The error value is saved in NtCurrentTeb()->TlsSlots[16] */
        ok_eq_ptr(p__errno(), &pteb->TlsSlots[16]);
    }

    /* Set to a known value before the test */
    pErrno = (int*)&pteb->TlsSlots[16];
    *pErrno = 0xdeadbeef;

    /* Pass -1 to sqrt to trigger _errno */
    psqrt(-1.);

    /* Check that errno is set to EDOM */
    ok(*pErrno == EDOM, "errno after sqrt(-1.) = %d, expected EDOM\n", *pErrno);
}

/*
 * Stress-test Vista+ condition variables (SleepConditionVariableCS/SRW),
 * which use the kernel keyed-event path.
 *
 * The pumper must not advance the published batch until all workers have
 * finished the previous round; otherwise the batch counter races ahead and
 * workers only observe one transition.
 */

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif

#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#include <windef.h>
#include <winbase.h>

static volatile LONG g_publish;
static volatile LONG g_ops;
static int g_total_batches;
static int g_num_workers;

static CRITICAL_SECTION g_cs;
static CONDITION_VARIABLE g_cv_worker;
static CONDITION_VARIABLE g_cv_pumper;

static unsigned int __stdcall worker_cs(void *unused)
{
    int iter;

    (void)unused;
    for (iter = 0; iter < g_total_batches; iter++)
    {
        EnterCriticalSection(&g_cs);
        while (g_publish <= iter)
            SleepConditionVariableCS(&g_cv_worker, &g_cs, INFINITE);
        LeaveCriticalSection(&g_cs);

        InterlockedIncrement(&g_ops);

        EnterCriticalSection(&g_cs);
        WakeConditionVariable(&g_cv_pumper);
        LeaveCriticalSection(&g_cs);
    }
    return 0;
}

static unsigned int __stdcall pumper_cs(void *unused)
{
    int b;

    (void)unused;
    for (b = 1; b <= g_total_batches; b++)
    {
        EnterCriticalSection(&g_cs);
        while (g_ops < (LONG)(((LONG64)b - 1) * g_num_workers))
            SleepConditionVariableCS(&g_cv_pumper, &g_cs, INFINITE);
        g_publish = b;
        WakeAllConditionVariable(&g_cv_worker);
        LeaveCriticalSection(&g_cs);
    }
    return 0;
}

static int run_cs_stress(int batches, int workers)
{
    HANDLE *threads;
    HANDLE pumper;
    int i;
    LONG expected;

    g_publish = 0;
    g_ops = 0;
    g_total_batches = batches;
    g_num_workers = workers;

    InitializeCriticalSection(&g_cs);
    InitializeConditionVariable(&g_cv_worker);
    InitializeConditionVariable(&g_cv_pumper);

    threads = calloc((size_t)workers, sizeof(HANDLE));
    if (!threads)
        return 1;

    /* Workers must block before the pumper publishes or they miss wakes. */
    for (i = 0; i < workers; i++)
    {
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, worker_cs, NULL, 0, NULL);
        if (!threads[i])
        {
            while (--i >= 0)
            {
                WaitForSingleObject(threads[i], INFINITE);
                CloseHandle(threads[i]);
            }
            free(threads);
            DeleteCriticalSection(&g_cs);
            return 1;
        }
    }

    pumper = (HANDLE)_beginthreadex(NULL, 0, pumper_cs, NULL, 0, NULL);
    if (!pumper)
    {
        for (i = 0; i < workers; i++)
        {
            WaitForSingleObject(threads[i], INFINITE);
            CloseHandle(threads[i]);
        }
        free(threads);
        DeleteCriticalSection(&g_cs);
        return 1;
    }

    WaitForSingleObject(pumper, INFINITE);
    CloseHandle(pumper);

    for (i = 0; i < workers; i++)
    {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
    }

    free(threads);
    DeleteCriticalSection(&g_cs);

    expected = (LONG)((LONG64)batches * workers);
    if (g_ops != expected)
    {
        wprintf(L"[keyedevt_stress] FAIL CS: completions=%ld expected=%ld\n", g_ops, expected);
        return 1;
    }
    wprintf(L"[keyedevt_stress] CS: %d batches x %d workers OK\n", batches, workers);
    return 0;
}

static SRWLOCK g_srw;
static CONDITION_VARIABLE g_cv_srw_worker;
static CONDITION_VARIABLE g_cv_srw_pumper;

static unsigned int __stdcall worker_srw(void *unused)
{
    int iter;

    (void)unused;
    for (iter = 0; iter < g_total_batches; iter++)
    {
        AcquireSRWLockExclusive(&g_srw);
        while (g_publish <= iter)
            SleepConditionVariableSRW(&g_cv_srw_worker, &g_srw, INFINITE, 0);
        ReleaseSRWLockExclusive(&g_srw);

        InterlockedIncrement(&g_ops);

        AcquireSRWLockExclusive(&g_srw);
        WakeConditionVariable(&g_cv_srw_pumper);
        ReleaseSRWLockExclusive(&g_srw);
    }
    return 0;
}

static unsigned int __stdcall pumper_srw(void *unused)
{
    int b;

    (void)unused;
    for (b = 1; b <= g_total_batches; b++)
    {
        AcquireSRWLockExclusive(&g_srw);
        while (g_ops < (LONG)(((LONG64)b - 1) * g_num_workers))
            SleepConditionVariableSRW(&g_cv_srw_pumper, &g_srw, INFINITE, 0);
        g_publish = b;
        WakeAllConditionVariable(&g_cv_srw_worker);
        ReleaseSRWLockExclusive(&g_srw);
    }
    return 0;
}

static int run_srw_stress(int batches, int workers)
{
    HANDLE *threads;
    HANDLE pumper;
    int i;
    LONG expected;

    g_publish = 0;
    g_ops = 0;
    g_total_batches = batches;
    g_num_workers = workers;

    InitializeSRWLock(&g_srw);
    InitializeConditionVariable(&g_cv_srw_worker);
    InitializeConditionVariable(&g_cv_srw_pumper);

    threads = calloc((size_t)workers, sizeof(HANDLE));
    if (!threads)
        return 1;

    for (i = 0; i < workers; i++)
    {
        threads[i] = (HANDLE)_beginthreadex(NULL, 0, worker_srw, NULL, 0, NULL);
        if (!threads[i])
        {
            while (--i >= 0)
            {
                WaitForSingleObject(threads[i], INFINITE);
                CloseHandle(threads[i]);
            }
            free(threads);
            return 1;
        }
    }

    pumper = (HANDLE)_beginthreadex(NULL, 0, pumper_srw, NULL, 0, NULL);
    if (!pumper)
    {
        for (i = 0; i < workers; i++)
        {
            WaitForSingleObject(threads[i], INFINITE);
            CloseHandle(threads[i]);
        }
        free(threads);
        return 1;
    }

    WaitForSingleObject(pumper, INFINITE);
    CloseHandle(pumper);

    for (i = 0; i < workers; i++)
    {
        WaitForSingleObject(threads[i], INFINITE);
        CloseHandle(threads[i]);
    }

    free(threads);

    expected = (LONG)((LONG64)batches * workers);
    if (g_ops != expected)
    {
        wprintf(L"[keyedevt_stress] FAIL SRW: completions=%ld expected=%ld\n", g_ops, expected);
        return 1;
    }
    wprintf(L"[keyedevt_stress] SRW: %d batches x %d workers OK\n", batches, workers);
    return 0;
}

int wmain(int argc, WCHAR **argv)
{
    int batches = 2000;
    int workers = 12;

    if (argc >= 2)
        batches = _wtoi(argv[1]);
    if (argc >= 3)
        workers = _wtoi(argv[2]);

    if (batches < 1)
        batches = 1;
    if (workers < 1)
        workers = 1;

    wprintf(L"keyedevt_stress: batches=%d workers=%d\n", batches, workers);

    if (run_cs_stress(batches, workers))
        return 1;
    if (run_srw_stress(batches, workers))
        return 1;

    wprintf(L"keyedevt_stress: all tests passed.\n");
    return 0;
}

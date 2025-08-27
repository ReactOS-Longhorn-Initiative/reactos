/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for WaitOnAddress / WakeByAddress APIs
 * COPYRIGHT:   Copyright 2025 <justin.miller@reactos.org>
 */

#include "precomp.h"

//=============================================================================
// Function Pointer Definitions
//=============================================================================

typedef
NTSTATUS
(NTAPI *FN_RtlWaitOnAddress)(
    _In_ volatile VOID* Address,
    _In_ PVOID CompareAddress,
    _In_ SIZE_T AddressSize,
    _In_opt_ PLARGE_INTEGER Timeout);

typedef
VOID
(NTAPI *FN_RtlWakeAddressSingle)(
    _In_ PVOID Address);

typedef
VOID
(NTAPI *FN_RtlWakeAddressAll)(
    _In_ PVOID Address);

static FN_RtlWaitOnAddress pfnRtlWaitOnAddress;
static FN_RtlWakeAddressSingle pfnRtlWakeAddressSingle;
static FN_RtlWakeAddressAll pfnRtlWakeAddressAll;


//=============================================================================
// Test Globals & Worker Threads
//=============================================================================

#define ALL_WAKE_THREAD_COUNT 4

static volatile LONG g_TestValue = 0;
static volatile BOOL g_WorkerThreadWokeUp = FALSE;
static volatile NTSTATUS g_WorkerStatus = STATUS_PENDING;
static volatile LONG g_WakeupCounter = 0;

static DWORD WINAPI SingleWakeWorkerThread(LPVOID lpParam)
{
    LONG CompareValue = 0;
    g_WorkerThreadWokeUp = FALSE;
    g_WorkerStatus = STATUS_PENDING;

    // Wait until g_TestValue is no longer 0.
    g_WorkerStatus = pfnRtlWaitOnAddress((PVOID)&g_TestValue, &CompareValue, sizeof(g_TestValue), NULL);

    // If we woke up successfully, the value should now be 1.
    if (g_TestValue == 1)
    {
        g_WorkerThreadWokeUp = TRUE;
    }

    return 0;
}

static DWORD WINAPI TimeoutWorkerThread(LPVOID lpParam)
{
    LONG CompareValue = 0;
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = -100 * 10000; // 100 ms

    g_WorkerStatus = STATUS_PENDING;

    // Wait for a value that will never change, expecting a timeout.
    g_WorkerStatus = pfnRtlWaitOnAddress((PVOID)&g_TestValue, &CompareValue, sizeof(g_TestValue), &Timeout);

    return 0;
}

static DWORD WINAPI AllWakeWorkerThread(LPVOID lpParam)
{
    LONG CompareValue = 0;
    NTSTATUS Status;

    // Wait until the main thread signals us to wake up.
    Status = pfnRtlWaitOnAddress((PVOID)&g_TestValue, &CompareValue, sizeof(g_TestValue), NULL);

    // If woken successfully, increment the shared counter.
    if (NT_SUCCESS(Status) && g_TestValue == 1)
    {
        InterlockedIncrement(&g_WakeupCounter);
    }

    return 0;
}


//=============================================================================
// Main Test Function
//=============================================================================

START_TEST(WakeAddress)
{
    HMODULE hNtDll;
    HANDLE hThread;
    HANDLE hThreads[ALL_WAKE_THREAD_COUNT];
    LONG CompareValue;

    // Load functions from ntdll
    hNtDll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtDll)
    {
        skip("GetModuleHandleW for ntdll failed with 0x%08lX\n", GetLastError());
        return;
    }
    pfnRtlWaitOnAddress = (FN_RtlWaitOnAddress)GetProcAddress(hNtDll, "RtlWaitOnAddress");
    pfnRtlWakeAddressSingle = (FN_RtlWakeAddressSingle)GetProcAddress(hNtDll, "RtlWakeAddressSingle");
    pfnRtlWakeAddressAll = (FN_RtlWakeAddressAll)GetProcAddress(hNtDll, "RtlWakeAddressAll");

    if (!pfnRtlWaitOnAddress || !pfnRtlWakeAddressSingle || !pfnRtlWakeAddressAll)
    {
        skip("RtlWakeAddress functions not found in ntdll.dll\n");
        return;
    }

    // --- Test 1: Basic WakeAddressSingle ---
    trace("Testing single wake...\n");
    InterlockedExchange(&g_TestValue, 0);
    hThread = CreateThread(NULL, 0, SingleWakeWorkerThread, NULL, 0, NULL);
    ok(hThread != NULL, "CreateThread failed with %lu\n", GetLastError());
    Sleep(100); // Give the worker thread time to start waiting.

    InterlockedExchange(&g_TestValue, 1);
    pfnRtlWakeAddressSingle((PVOID)&g_TestValue);
    WaitForSingleObject(hThread, INFINITE);

    ok_eq_bool(g_WorkerThreadWokeUp, TRUE);
    ok_eq_long(g_WorkerStatus, STATUS_SUCCESS);
    CloseHandle(hThread);

    // --- Test 2: No wait if values mismatch ---
    trace("Testing value mismatch (no wait)...\n");
    InterlockedExchange(&g_TestValue, 0);
    CompareValue = 1; // Different from g_TestValue
    g_WorkerStatus = pfnRtlWaitOnAddress((PVOID)&g_TestValue, &CompareValue, sizeof(g_TestValue), NULL);
    ok_eq_long(g_WorkerStatus, STATUS_SUCCESS);

    // --- Test 3: Timeout ---
    trace("Testing timeout...\n");
    InterlockedExchange(&g_TestValue, 0);
    hThread = CreateThread(NULL, 0, TimeoutWorkerThread, NULL, 0, NULL);
    ok(hThread != NULL, "CreateThread failed with %lu\n", GetLastError());
    WaitForSingleObject(hThread, INFINITE);
    ok_eq_long(g_WorkerStatus, STATUS_TIMEOUT);
    CloseHandle(hThread);

    // --- Test 4: WakeAddressAll ---
    trace("Testing wake all...\n");
    InterlockedExchange(&g_TestValue, 0);
    InterlockedExchange(&g_WakeupCounter, 0);
    for (int i = 0; i < ALL_WAKE_THREAD_COUNT; i++)
    {
        hThreads[i] = CreateThread(NULL, 0, AllWakeWorkerThread, NULL, 0, NULL);
        ok(hThreads[i] != NULL, "CreateThread %d failed with %lu\n", i, GetLastError());
    }
    Sleep(200); // Give all threads time to start waiting.

    InterlockedExchange(&g_TestValue, 1);
    pfnRtlWakeAddressAll((PVOID)&g_TestValue);
    WaitForMultipleObjects(ALL_WAKE_THREAD_COUNT, hThreads, TRUE, INFINITE);

    ok_eq_long((long)g_WakeupCounter, (long)ALL_WAKE_THREAD_COUNT);
    for (int i = 0; i < ALL_WAKE_THREAD_COUNT; i++)
    {
        CloseHandle(hThreads[i]);
    }
}

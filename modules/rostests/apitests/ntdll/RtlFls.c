/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for RtlFlsAlloc, RtlFlsFree, RtlFlsGetValue, RtlFlsSetValue
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

//
// References:
// - https://github.com/DynamoRIO/dynamorio/blob/master/core/win32/drwinapi/ntdll_redir.c
// - https://dronesec.net/blog/code-execution-via-fiber-local-storage/
// - https://ntquery.wordpress.com/2014/03/29/anti-debug-fiber-local-storage-fls/
//

typedef struct _FLS_CALLBACK_INFO
{
    PFLS_CALLBACK_FUNCTION Callback;
    PVOID Unknown;
} FLS_CALLBACK_INFO, *PFLS_CALLBACK_INFO;

typedef
NTSTATUS
NTAPI
FN_RtlFlsAlloc(
    _In_ PFLS_CALLBACK_FUNCTION Callback,
    _Out_ PULONG FlsIndex);

typedef
NTSTATUS
NTAPI
FN_RtlFlsAllocEx(
    _In_ PFLS_CALLBACK_FUNCTION Callback,
    _Out_ PULONG,
    _Out_ PULONG FlsIndex);

typedef
NTSTATUS
NTAPI
FN_RtlFlsFree(
    _In_ ULONG FlsIndex);

typedef
NTSTATUS
NTAPI
FN_RtlFlsGetValue(
    _In_ ULONG FlsIndex,
    _Out_ PVOID* FlsData);

typedef
PVOID
WINAPI
FN_RtlFlsGetValue2(
    _In_ ULONG FlsIndex);

typedef
NTSTATUS
NTAPI
FN_RtlFlsSetValue(
    _In_ ULONG FlsIndex,
    _In_ PVOID FlsData);

typedef
NTSTATUS
NTAPI
FN_RtlProcessFlsData_Vista(
    _In_ PRTL_FLS_DATA FlsData);

typedef
NTSTATUS
NTAPI
FN_RtlProcessFlsData_Win10(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* FlsData);

FN_RtlFlsAlloc* pRtlFlsAlloc;
FN_RtlFlsAllocEx* pRtlFlsAllocEx;
FN_RtlFlsFree* pRtlFlsFree;
FN_RtlFlsGetValue* pRtlFlsGetValue;
FN_RtlFlsGetValue2* pRtlFlsGetValue2;
FN_RtlFlsSetValue* pRtlFlsSetValue;
FN_RtlProcessFlsData_Vista* pRtlProcessFlsData;

static ULONG g_SharedFlsIndex1;
static ULONG g_SharedFlsIndex2;

static void InitFunctionPtrs(void)
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");

#define INIT_FN_PTR(func) \
    p##func = (void*)GetProcAddress(hNtdll, #func); \
    if (p##func == NULL) \
    { \
        trace("%s not found\n", #func); \
    }

    INIT_FN_PTR(RtlFlsAlloc);
    INIT_FN_PTR(RtlFlsAllocEx);
    INIT_FN_PTR(RtlFlsFree);
    INIT_FN_PTR(RtlFlsGetValue);
    INIT_FN_PTR(RtlFlsGetValue2);
    INIT_FN_PTR(RtlFlsSetValue);
    INIT_FN_PTR(RtlProcessFlsData);
}

typedef struct _RTL_FLS_DATA_VISTA
{
    LIST_ENTRY ListEntry;
    PVOID Data[128];
} RTL_FLS_DATA_VISTA, *PRTL_FLS_DATA_VISTA;

typedef struct _RTLP_FLS_VALUE_ARRAY
{
    ULONG64 Unknown1;
    ULONG64 Unknown2;
    PVOID Elements[510]; // The size doubles per index
} RTLP_FLS_VALUE_ARRAY, *PRTLP_FLS_VALUE_ARRAY;

typedef struct _RTL_FLS_DATA_WIN10
{
    LIST_ENTRY ListEntry;
    PRTLP_FLS_VALUE_ARRAY ValueArrays[8];
} RTL_FLS_DATA_WIN10, *PRTL_FLS_DATA_WIN10;


static ULONG FlsCallbackCount = 0;

void NTAPI FlsTestCallback(PVOID lpFlsData)
{
    (void)lpFlsData;
    FlsCallbackCount++;
}

void Test_RtlFlsAllocFree(void)
{
    PTEB teb = NtCurrentTeb();
    PPEB peb = NtCurrentPeb();
    ULONG flsIndex1, flsIndex2, flsIndexFirst;
    NTSTATUS status;

    if ((pRtlFlsAlloc == NULL) || (pRtlFlsFree == NULL))
    {
        skip("RtlFlsAlloc or RtlFlsFree not available\n");
        return;
    }

    /* We expect an access violation, if the 2nd parameter is NULL */
    StartSeh()
        status = pRtlFlsAlloc(NULL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Allocate first index without a callback */
    status = pRtlFlsAlloc(NULL, &flsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok(flsIndex1 >= 3 && flsIndex1 <= 8, "Unexpected first FLS index: %lu\n", flsIndex1);
    flsIndexFirst = flsIndex1;

    /* Allocate 2nd index with a callback */
    status = pRtlFlsAlloc(FlsTestCallback, &flsIndex2);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(flsIndex2, flsIndexFirst + 1);

    /* Try to free invalid indices */
    status = pRtlFlsFree(0);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);
    status = pRtlFlsFree(5000);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);

    /* Try to free an index that was not allocated */
    status = pRtlFlsFree(flsIndex2 + 1);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);

    /* Delete first FLS slot, should not trigger callback */
    status = pRtlFlsFree(flsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(FlsCallbackCount, 0);

    /* Double free slot 1 */
    status = pRtlFlsFree(flsIndex1);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);

    /* Allocate index without a callback and set the value */
    status = pRtlFlsAlloc(NULL, &flsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(flsIndex1, (GetNTVersion() >= _WIN32_WINNT_WIN10) ? flsIndex2 + 1 : flsIndexFirst);
    FlsSetValue(flsIndex1, (PVOID)0x1234);

    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        PRTL_FLS_DATA_WIN10 flsData = (PRTL_FLS_DATA_WIN10)teb->FlsData;
        ok_eq_pointer(flsData->ValueArrays[0]->Elements[flsIndex1 - 1], (PVOID)0x1234);
    }
    else
    {
        PRTL_FLS_DATA_VISTA flsData = (PRTL_FLS_DATA_VISTA)teb->FlsData;
        ok_eq_pointer(flsData->Data[flsIndex1], (PVOID)0x1234);
        ok_eq_ulong(peb->FlsHighIndex, flsIndex2);
        PFLS_CALLBACK_INFO callbackInfo = (PFLS_CALLBACK_INFO)peb->FlsCallback;
        ok((callbackInfo == NULL) || (callbackInfo[flsIndex1].Callback == NULL), "Unexpected callback\n");
    }

    /* Delete flsIndex1, should not trigger callback */
    status = pRtlFlsFree(flsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(FlsCallbackCount, 0);

    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        PRTL_FLS_DATA_WIN10 flsData = (PRTL_FLS_DATA_WIN10)teb->FlsData;
        ok_eq_pointer(flsData->ValueArrays[0]->Elements[flsIndex1 - 1], NULL);
    }
    else
    {
        PRTL_FLS_DATA_VISTA flsData = (PRTL_FLS_DATA_VISTA)teb->FlsData;
        ok_eq_pointer(flsData->Data[flsIndex1 - 1], NULL);
    }

    /* Delete flsIndex2, should not trigger callback */
    status = pRtlFlsFree(flsIndex2);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(FlsCallbackCount, 0);

    /* Allocate index with a callback and set the value */
    status = pRtlFlsAlloc(FlsTestCallback, &flsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(flsIndex1, flsIndexFirst);
    FlsSetValue(flsIndex1, (PVOID)(ULONG_PTR)0x1234);

    if (GetNTVersion() < _WIN32_WINNT_WIN10)
    {
        PFLS_CALLBACK_INFO callbackInfo = (PFLS_CALLBACK_INFO)peb->FlsCallback;
        ok(callbackInfo != NULL, "peb->FlsCallback is NULL. Bye.\n");
        ok_eq_ptr(callbackInfo[flsIndex1].Callback, FlsTestCallback);
    }

    /* Delete FLS slot, this should now trigger callback */
    status = pRtlFlsFree(flsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(FlsCallbackCount, 1);
    FlsCallbackCount = 0;

    if (GetNTVersion() < _WIN32_WINNT_WIN10)
    {
        PFLS_CALLBACK_INFO callbackInfo = (PFLS_CALLBACK_INFO)peb->FlsCallback;
        ok_eq_ptr(callbackInfo[flsIndex1].Callback, NULL);
    }

    /* Allocate index with a callback and set the value, then set it to NULL */
    status = pRtlFlsAlloc(FlsTestCallback, &flsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(flsIndex1, flsIndexFirst);
    FlsSetValue(flsIndex1, (PVOID)0x1234);
    FlsSetValue(flsIndex1, NULL);

    /* Delete FLS slot, should not trigger callback */
    status = pRtlFlsFree(flsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(FlsCallbackCount, 0);

    /* Test how many slots we can allocate */
    ULONG NumberAllocated = 0;
    while (status = pRtlFlsAlloc(NULL, &flsIndex2), NT_SUCCESS(status))
    {
        ok(flsIndex2 == flsIndexFirst + NumberAllocated, "Invalid index: %lu != %lu + %lu\n", flsIndex2, flsIndex1, NumberAllocated);
        NumberAllocated++;
    }

    ok_ntstatus(status, STATUS_NO_MEMORY);

    /* Final index is not overwritten */
    ok(flsIndex2 == flsIndexFirst + NumberAllocated - 1, "Invalid index: %lu != %lu + %lu\n", flsIndex2, flsIndex1, NumberAllocated);

    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        ok_eq_ulong(NumberAllocated, 4080 - flsIndexFirst);
    }
    else
    {
        ok_eq_ulong(NumberAllocated, 128 - flsIndexFirst);
    }

    /* Free all allocated slots */
    for (ULONG i = 0; i < NumberAllocated; i++)
    {
        status = pRtlFlsFree(flsIndex1 + i);
        ok_ntstatus(status, STATUS_SUCCESS);
    }

    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        /* All arrays ares still allocated */
        PRTL_FLS_DATA_WIN10 flsData = (PRTL_FLS_DATA_WIN10)teb->FlsData;
        for (ULONG i = 0; i < 8; i++)
        {
            ok(flsData->ValueArrays[i] != NULL, "flsData->ValueArrays[%lu] is NULL\n", i);
        }
    }
    else
    {
        ok_eq_ulong(peb->FlsHighIndex, 127);
    }

    /* Free the 2 shared indices */
    status = pRtlFlsFree(g_SharedFlsIndex1);
    ok_ntstatus(status, STATUS_SUCCESS);
    status = pRtlFlsFree(g_SharedFlsIndex2);
    ok_ntstatus(status, STATUS_SUCCESS);
}

void Test_RtlFlsAllocEx(void)
{
    if (pRtlFlsAllocEx == NULL)
    {
        skip("RtlFlsAllocEx not available\n");
        return;
    }

    // TODO
}

void Test_RtlFlsGetSetValue(void)
{
    PTEB teb = NtCurrentTeb();
    PVOID value;
    BOOL ret;
    NTSTATUS status;

    /* Test getting index 0 */
    SetLastError(0xdeadbeef);
    value = FlsGetValue(0);
    ok_eq_pointer(value, NULL);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test setting index 0 */
    SetLastError(0xdeadbeef);
    ret = FlsSetValue(0, (PVOID)0x1234);
    ok_eq_bool(ret, FALSE);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test getting index 5000 */
    SetLastError(0xdeadbeef);
    value = FlsGetValue(5000);
    ok_eq_pointer(value, NULL);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test setting index 5000 */
    SetLastError(0xdeadbeef);
    ret = FlsSetValue(5000, (PVOID)0x1234);
    ok_eq_bool(ret, FALSE);
    ok_eq_ulong(GetLastError(), ERROR_INVALID_PARAMETER);

    /* Test setting index 77 (not allocated) */
    SetLastError(0xdeadbeef);
    ret = FlsSetValue(77, (PVOID)0x1234);
    ok_eq_bool(ret, TRUE);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test getting index 77 (not allocated) */
    SetLastError(0xdeadbeef);
    value = FlsGetValue(77);
    ok_eq_pointer(value, (PVOID)0x1234);
    ok_eq_ulong(GetLastError(), ERROR_SUCCESS);

    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        PRTL_FLS_DATA_WIN10 flsData = (PRTL_FLS_DATA_WIN10)teb->FlsData;
        ok_eq_pointer(flsData->ValueArrays[2]->Elements[28], (PVOID)0x1234);
    }
    else
    {
        PRTL_FLS_DATA_VISTA flsData = (PRTL_FLS_DATA_VISTA)teb->FlsData;
        ok_eq_pointer(flsData->Data[77], (PVOID)0x1234);
    }

    /* Free index 77 (fails, as it was never allocated) */
    ret = FlsFree(77);
    ok_eq_bool(ret, FALSE);

    if ((pRtlFlsGetValue == NULL) || (pRtlFlsSetValue == NULL))
    {
        skip("RtlFlsGetValue or pRtlFlsSetValue not available\n");
        return;
    }

    /* Test setting index 0 */
    SetLastError(0xdeadbeef);
    status = pRtlFlsSetValue(0, (PVOID)0x1234);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test getting index 0 */
    SetLastError(0xdeadbeef);
    value = (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull;
    status = pRtlFlsGetValue(0, &value);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);
    ok_eq_pointer(value, (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test setting index 5000 */
    SetLastError(0xdeadbeef);
    status = pRtlFlsSetValue(5000, (PVOID)0x1234);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test getting index 5000 */
    SetLastError(0xdeadbeef);
    value = (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull;
    status = pRtlFlsGetValue(5000, &value);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);
    ok_eq_pointer(value, (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test getting index 99 (not allocated, not set) */
    SetLastError(0xdeadbeef);
    value = (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull;
    status = pRtlFlsGetValue(99, &value);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_pointer(value, NULL);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test setting index 99 (not allocated) */
    SetLastError(0xdeadbeef);
    status = pRtlFlsSetValue(99, (PVOID)(ULONG_PTR)0x1234);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    /* Test getting index 99 (not allocated) */
    SetLastError(0xdeadbeef);
    value = (PVOID)(ULONG_PTR)0xDEADBEEFDEADBEEFull;
    status = pRtlFlsGetValue(99, &value);
    ok_ntstatus(status, STATUS_SUCCESS);
    ok_eq_pointer(value, (PVOID)(ULONG_PTR)0x1234);
    ok_eq_ulong(GetLastError(), 0xdeadbeef);

    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        PRTL_FLS_DATA_WIN10 flsData = (PRTL_FLS_DATA_WIN10)teb->FlsData;
        ok_eq_pointer(flsData->ValueArrays[2]->Elements[50], (PVOID)0x1234);
    }
    else
    {
        PRTL_FLS_DATA_VISTA flsData = (PRTL_FLS_DATA_VISTA)teb->FlsData;
        ok_eq_pointer(flsData->Data[99], (PVOID)(ULONG_PTR)0x1234);
    }

    /* Free index 99 (fails, as it was never allocated) */
    status = pRtlFlsFree(99);
    ok_ntstatus(status, STATUS_INVALID_PARAMETER);
}

void Test_RtlFlsGetValue2(void)
{
    if (pRtlFlsGetValue2 == NULL)
    {
        skip("RtlFlsGetValue2 not available\n");
        return;
    }

    // TODO
}

void Test_RtlProcessFlsData_Vista(void)
{
    if (pRtlProcessFlsData == NULL)
    {
        skip("RtlProcessFlsData not available\n");
        return;
    }

    // TODO
}

void Test_RtlProcessFlsData_Win10(void)
{
    FN_RtlProcessFlsData_Win10* pRtlProcessFlsData_Win10 = (FN_RtlProcessFlsData_Win10*)pRtlProcessFlsData;
    if (pRtlProcessFlsData == NULL)
    {
        skip("RtlProcessFlsData not available\n");
        return;
    }

    // TODO
    DBG_UNREFERENCED_LOCAL_VARIABLE(pRtlProcessFlsData_Win10);
}

void Test_RtlProcessFlsData(void)
{
    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        Test_RtlProcessFlsData_Win10();
    }
    else
    {
        Test_RtlProcessFlsData_Vista();
    }
}

static DWORD WINAPI TestThreadProc(LPVOID lpParameter)
{
    PTEB teb = NtCurrentTeb();
    PPEB peb = NtCurrentPeb();

    /* Test the initial state */
    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        /* Windows 10 doesn't use the PEB fields anymore */
        ok_eq_pointer(peb->FlsCallback, NULL);
        ok_eq_pointer(peb->FlsBitmap, NULL);
        ok_eq_ulong(peb->FlsBitmapBits[0], 0);
        ok_eq_ulong(peb->FlsBitmapBits[1], 0);
        ok_eq_ulong(peb->FlsBitmapBits[2], 0);
        ok_eq_ulong(peb->FlsBitmapBits[3], 0);
        ok_eq_ulong(peb->FlsHighIndex, 0);
        ok_eq_pointer(peb->FlsListHead.Flink, NULL);
        ok_eq_pointer(peb->FlsListHead.Blink, NULL);

        ok(teb->FlsData != NULL, "teb->FlsData is NULL\n");
        if (teb->FlsData != NULL)
        {
            PRTL_FLS_DATA_WIN10 flsData = (PRTL_FLS_DATA_WIN10)teb->FlsData;
            ok(flsData->ListEntry.Flink != NULL, "flsData->ListEntry.Flink is NULL\n");
            ok(flsData->ListEntry.Blink != NULL, "flsData->ListEntry.Blink is NULL\n");

            /* Windows 10 a 2-level array table */
            ok(flsData->ValueArrays[0] != NULL, "flsData->ValueArrays[0] is NULL\n");
            ok(flsData->ValueArrays[1] == NULL, "flsData->ValueArrays[1] is not NULL\n");
            ok(flsData->ValueArrays[0]->Unknown1 == 0, "flsData->ValueArrays[0]->Unknown1 is not 0\n");
            ok(flsData->ValueArrays[0]->Unknown2 == 0, "flsData->ValueArrays[0]->Unknown2 is not 0\n");
            ok(flsData->ValueArrays[0]->Elements[0] != NULL, "flsData->ValueArrays[0]->Elements[0] is NULL\n");
        }
    }
    else
    {
        ok(peb->FlsCallback != NULL, "peb->FlsCallback is NULL\n");
        ok(peb->FlsBitmap != NULL, "peb->FlsBitmap is NULL\n");
        ok(peb->FlsBitmapBits[0] > 0 && peb->FlsBitmapBits[0] < 128,
           "Unexpected value for peb->FlsBitmapBits[0]: 0x%lx\n", peb->FlsBitmapBits[0]);
        ok_eq_ulong(peb->FlsBitmapBits[1], 0);
        ok_eq_ulong(peb->FlsBitmapBits[2], 0);
        ok_eq_ulong(peb->FlsBitmapBits[3], 0);
        ok(peb->FlsHighIndex > 0 && peb->FlsHighIndex < 128,
           "Unexpected value for peb->FlsHighIndex: 0x%lx\n", peb->FlsHighIndex);
        ok(peb->FlsListHead.Flink != NULL, "peb->FlsListHead.Flink is NULL\n");
        ok(peb->FlsListHead.Blink != NULL, "peb->FlsListHead.Blink is NULL\n");
        todo_ros ok(teb->FlsData != NULL, "teb->FlsData == %p\n", teb->FlsData);
        if (teb->FlsData != NULL)
        {
            PRTL_FLS_DATA_VISTA flsData = (PRTL_FLS_DATA_VISTA)teb->FlsData;
            ok(flsData->ListEntry.Flink != NULL, "flsData->ListEntry.Flink is NULL\n");
            ok(flsData->ListEntry.Blink != NULL, "flsData->ListEntry.Blink is NULL\n");
            ok_eq_pointer(flsData->ListEntry.Flink, &peb->FlsListHead);
            ok(flsData->Data[127] == NULL, "flsData->Data[127] is not NULL\n");
        }
    }

    Test_RtlFlsAllocFree();
    Test_RtlFlsAllocEx();
    Test_RtlFlsGetSetValue();
    Test_RtlFlsGetValue2();
    Test_RtlProcessFlsData();

    return 0;
}


START_TEST(RtlFls)
{
    HANDLE hThread;

    InitFunctionPtrs();

    if (GetNTVersion() >= _WIN32_WINNT_VISTA)
    {
        ok(pRtlFlsAlloc != NULL, "pRtlFlsAlloc is NULL\n");
        ok(pRtlFlsFree != NULL, "pRtlFlsFree is NULL\n");
        ok(pRtlProcessFlsData != NULL, "pRtlProcessFlsData is NULL\n");
    }

    if (GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        ok(pRtlFlsGetValue != NULL, "pRtlFlsGetValue is NULL\n");
        ok(pRtlFlsSetValue != NULL, "pRtlFlsSetValue is NULL\n");
    }

    /* Allocate 2 global FLS indices, one without, one with callback */
    g_SharedFlsIndex1 = FlsAlloc(NULL);
    ok((g_SharedFlsIndex1 >= 1) && (g_SharedFlsIndex1 <= 6), "Unexpected first FLS index: %lu\n", g_SharedFlsIndex1);
    FlsSetValue(g_SharedFlsIndex1, (PVOID)(ULONG_PTR)0xdead0001);
    g_SharedFlsIndex2 = FlsAlloc(FlsTestCallback);
    ok((g_SharedFlsIndex2 >= 2) && (g_SharedFlsIndex2 <= 7), "Unexpected first FLS index: %lu\n", g_SharedFlsIndex2);
    FlsSetValue(g_SharedFlsIndex2, (PVOID)(ULONG_PTR)0xdead0002);

    /* Create a test thread to test FLS in a different thread context */
    hThread = CreateThread(NULL, 0, TestThreadProc, NULL, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    /* Check the shared indices */
    if (GetNTVersion() < _WIN32_WINNT_WIN10)
    {
        PPEB peb = NtCurrentPeb();
        PTEB teb = NtCurrentTeb();;
        PFLS_CALLBACK_INFO callbackInfo = (PFLS_CALLBACK_INFO)peb->FlsCallback;
        ok_eq_ptr(callbackInfo[g_SharedFlsIndex1].Callback, NULL);
        ok_eq_ptr(callbackInfo[g_SharedFlsIndex2].Callback, NULL);
        PRTL_FLS_DATA_VISTA flsData = (PRTL_FLS_DATA_VISTA)teb->FlsData;
        ok_eq_pointer(flsData->Data[g_SharedFlsIndex1], NULL);
        ok_eq_pointer(flsData->Data[g_SharedFlsIndex2], NULL);
    }
}

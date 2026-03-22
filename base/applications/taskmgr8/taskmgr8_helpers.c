/*
 * String loading, locale-aware formatting, and small registry / CPUID queries.
 */

#include "taskmgr8_helpers.h"
#include "taskmgr8_shared.h"

#if ROS_HAVE_CPUID
#include <intrin.h>
#endif

void
LoadStr(UINT id, WCHAR *buf, size_t cch)
{
    if (LoadStringW(s_hInst, id, buf, (int)cch) <= 0)
        buf[0] = 0;
}

void
FormatSpeedFromMhz(DWORD mhz, WCHAR *out, size_t cch)
{
    /* NBSP keeps "4.30 GHz" on one line in multiline statics (no wrap before unit). */
    if (mhz >= 1000)
        StringCchPrintfW(out, cch, L"%.2f\u00A0GHz", mhz / 1000.0);
    else if (mhz)
        StringCchPrintfW(out, cch, L"%lu\u00A0MHz", mhz);
    else
        out[0] = 0;
}

void
FormatUptimeString(WCHAR *buf, size_t cch)
{
    /*
     * i386 MSVC CRT may omit __aullrem; avoid ULONGLONG printf on 32-bit.
     */
#if defined(_WIN64)
    ULONGLONG sec;

    if (!s_pfnGetTickCount64)
    {
        HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
        if (k32)
            s_pfnGetTickCount64 = (PFN_GetTickCount64)(void *)GetProcAddress(k32, "GetTickCount64");
    }
    if (s_pfnGetTickCount64)
        sec = s_pfnGetTickCount64() / 1000ULL;
    else
        sec = GetTickCount() / 1000U;

    StringCchPrintfW(buf, cch, L"%llu:%02llu:%02llu:%02llu",
                     sec / 86400ULL, (sec / 3600ULL) % 24ULL, (sec / 60ULL) % 60ULL, sec % 60ULL);
#else
    DWORD sec = GetTickCount() / 1000U;
    DWORD s = sec % 60U;
    DWORD m = (sec / 60U) % 60U;
    DWORD h = (sec / 3600U) % 24U;
    DWORD d = sec / 86400U;

    StringCchPrintfW(buf, cch, L"%lu:%02lu:%02lu:%02lu", d, h, m, s);
#endif
}

DWORD
CountPhysicalCores(void)
{
    DWORD len = 0, i, n;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buf;
    DWORD cores = 0;

    if (!GetLogicalProcessorInformation(NULL, &len) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return 0;
    if (len == 0)
        return 0;
    buf = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    if (!buf)
        return 0;
    if (!GetLogicalProcessorInformation(buf, &len))
    {
        HeapFree(GetProcessHeap(), 0, buf);
        return 0;
    }
    n = len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    for (i = 0; i < n; i++)
    {
        if (buf[i].Relationship == RelationProcessorCore)
            cores++;
    }
    HeapFree(GetProcessHeap(), 0, buf);
    return cores;
}

BOOL
CpuVirtHardwarePresent(void)
{
#if ROS_HAVE_CPUID
    int info[4];
    int maxLeaf;
    char vendor[13];

    __cpuid(info, 0);
    maxLeaf = info[0];
    CopyMemory(vendor + 0, &info[1], sizeof(int));
    CopyMemory(vendor + 4, &info[3], sizeof(int));
    CopyMemory(vendor + 8, &info[2], sizeof(int));
    vendor[12] = 0;

    __cpuid(info, 1);
    if (memcmp(vendor, "GenuineIntel", 12) == 0)
        return (((unsigned)info[2] >> 5) & 1u) != 0; /* VMX */
    if (memcmp(vendor, "AuthenticAMD", 12) == 0 || memcmp(vendor, "HygonGenuine", 12) == 0)
    {
        if (maxLeaf >= 0x80000000)
        {
            __cpuid(info, 0x80000000);
            if ((unsigned)info[0] >= 0x80000001u)
            {
                __cpuid(info, 0x80000001);
                return (((unsigned)info[2] >> 2) & 1u) != 0; /* SVM */
            }
        }
        return FALSE;
    }
    if (((unsigned)info[2] & (1u << 5)) != 0)
        return TRUE;
    if (maxLeaf >= 0x80000000)
    {
        __cpuid(info, 0x80000000);
        if ((unsigned)info[0] >= 0x80000001u)
        {
            __cpuid(info, 0x80000001);
            if (((unsigned)info[2] & (1u << 2)) != 0)
                return TRUE;
        }
    }
#endif
    return FALSE;
}

static void
FormatCacheKbHuman(DWORD kb, WCHAR *dst, size_t cch)
{
    if (!dst || cch == 0)
        return;
    if (kb == 0)
    {
        dst[0] = 0;
        return;
    }
    if (kb >= 1024)
        StringCchPrintfW(dst, cch, L"%.1f MB", (double)kb / 1024.0);
    else
        StringCchPrintfW(dst, cch, L"%lu KB", (ULONG)kb);
}

void
AppendCacheKvLine(WCHAR *lblDest, size_t cchLbl, WCHAR *valDest, size_t cchVal, UINT idsLbl, DWORD kb)
{
    WCHAR name[48], val[72];
    if (kb == 0)
        return;
    LoadStr(idsLbl, name, _countof(name));
    StringCchCatW(lblDest, cchLbl, name);
    StringCchCatW(lblDest, cchLbl, L":\r\n");
    FormatCacheKbHuman(kb, val, _countof(val));
    StringCchCatW(valDest, cchVal, val);
    StringCchCatW(valDest, cchVal, L"\r\n");
}

BOOL
ReadProcessor0CacheValue(PCWSTR valueName, DWORD *outKb)
{
    HKEY hKey;
    BYTE buf[96];
    DWORD cb = sizeof(buf), typ = 0;

    *outKb = 0;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS)
        return FALSE;
    if (RegQueryValueExW(hKey, valueName, NULL, &typ, buf, &cb) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    RegCloseKey(hKey);

    if (typ == REG_DWORD && cb >= sizeof(DWORD))
    {
        *outKb = *(DWORD *)buf;
        return *outKb != 0;
    }
    if ((typ == REG_SZ || typ == REG_EXPAND_SZ) && cb >= sizeof(WCHAR))
    {
        WCHAR *s = (WCHAR *)buf;
        size_t nwc = cb / sizeof(WCHAR);
        if (nwc >= sizeof(buf) / sizeof(WCHAR))
            nwc = sizeof(buf) / sizeof(WCHAR) - 1;
        s[nwc] = 0;
        *outKb = wcstoul(s, NULL, 0);
        return *outKb != 0;
    }
    return FALSE;
}

#if ROS_HAVE_CPUID
void
AppendMissingCachesFromCpuid4(WCHAR *specLbl, size_t cchLbl, WCHAR *specVal, size_t cchVal, int haveL1,
                              int haveL2, int haveL3)
{
    int i;
    double l1Bytes = 0, l2Bytes = 0, l3Bytes = 0;
    int maxf[4];

    __cpuid(maxf, 0);
    if ((unsigned)maxf[0] < 4)
        return;

    for (i = 0; i < 32; i++)
    {
        int r[4];
        unsigned type, level;
        unsigned ways, parts, ls;
        unsigned sets;
        double bytes;

        __cpuidex(r, 4, i);
        type = (unsigned)r[0] & 0x1Fu;
        if (type == 0)
            break;
        level = ((unsigned)r[0] >> 5) & 7u;
        ways = ((unsigned)r[2] >> 22) + 1u;
        parts = (((unsigned)r[2] >> 12) & 0x3FFu) + 1u;
        ls = ((unsigned)r[2] & 0xFFFu) + 1u;
        sets = (unsigned)r[3] + 1u;
        bytes = (double)ways * (double)parts * (double)ls * (double)sets;
        if (level == 1)
        {
            if (type == 1 || type == 2 || type == 3)
                l1Bytes += bytes;
        }
        else if (level == 2)
        {
            if (bytes > l2Bytes)
                l2Bytes = bytes;
        }
        else if (level == 3)
        {
            if (bytes > l3Bytes)
                l3Bytes = bytes;
        }
    }
    if (!haveL1 && l1Bytes > 0)
        AppendCacheKvLine(specLbl, cchLbl, specVal, cchVal, IDS_LBL_L1, (DWORD)(l1Bytes / 1024.0 + 0.5));
    if (!haveL2 && l2Bytes > 0)
        AppendCacheKvLine(specLbl, cchLbl, specVal, cchVal, IDS_LBL_L2, (DWORD)(l2Bytes / 1024.0 + 0.5));
    if (!haveL3 && l3Bytes > 0)
        AppendCacheKvLine(specLbl, cchLbl, specVal, cchVal, IDS_LBL_L3, (DWORD)(l3Bytes / 1024.0 + 0.5));
}
#endif

void
AppendSpecLbl(WCHAR *buf, size_t cch, UINT id)
{
    WCHAR t[72];
    LoadStr(id, t, _countof(t));
    StringCchCatW(buf, cch, t);
    StringCchCatW(buf, cch, L":\r\n");
}

void
FormatULongGrouped(ULONG n, WCHAR *buf, size_t cch)
{
    WCHAR tmp[24], grp[32];
    NUMBERFMTW fmt;

    StringCchPrintfW(tmp, _countof(tmp), L"%lu", n);
    fmt.NumDigits = 0;
    fmt.LeadingZero = FALSE;
    fmt.Grouping = 3;
    fmt.lpDecimalSep = L".";
    fmt.lpThousandSep = L",";
    fmt.NegativeOrder = 0;
    if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, tmp, &fmt, grp, (int)_countof(grp)))
        StringCchCopyW(buf, cch, grp);
    else
        StringCchCopyW(buf, cch, tmp);
}

#include "PresentationDebug.h"
#include <strsafe.h>
#include <debug.h>

#define TAG_NONAME              0x01
#define TAG_NONEWLINE           0x02
#define ARRAY_SIZE(x)           (sizeof(x) / sizeof(x[0]))

#define local_tagDefault              ((TRACETAG)0)
#define local_tagError                ((TRACETAG)1)
#define local_tagWarning              ((TRACETAG)2)
#define local_tagThread               ((TRACETAG)3)
#define local_tagAssertExit           ((TRACETAG)4)
#define local_tagAssertStacks         ((TRACETAG)5)
#define local_tagMemoryStrict         ((TRACETAG)6)
#define local_tagCoMemoryStrict       ((TRACETAG)7)
#define local_tagMemoryStrictTail     ((TRACETAG)8)
#define local_tagMemoryStrictAlign    ((TRACETAG)9)
#define local_tagOLEWatch             ((TRACETAG)10)
#define local_tagFALSE                ((TRACETAG)12)


struct TAGINFO g_rgtaginfo[] =
{
    { "Debug",      "General debug output",                     TRUE    },  //  0: tagDefault
    { "Trace",      "Errors",                                   TRUE    },  //  1: tagError
    { "Trace",      "Warnings",                                 FALSE   },  //  2: tagWarning
    { "Thread",     "Thread related tracing",                   FALSE   },  //  3: tagThread
    { "Assert",     "Exit on asserts",                          FALSE   },  //  4: tagAssertExit
    { "Assert",     "Stacktraces on asserts",                   TRUE    },  //  5: tagAssertStacks
    { "Memory",     "Use VMem for MemAlloc",                    FALSE   },  //  6: tagMemoryStrict
    { "Memory",     "Use VMem for CoTaskMemAlloc",              FALSE   },  //  7: tagCoMemoryStrict
    { "Memory",     "Use VMem strict at end (vs beginning)",    FALSE   },  //  8: tagMemoryStrictTail
    { "Memory",     "VMem pad to quadword at end",              FALSE   },  //  9: tagMemoryStrictAlign
    { "Trace",      "All calls to OCX interfaces",              FALSE   },  // 10: tagOLEWatch 
    { "FALSE",      "FALSE",                                    FALSE   },  // 12: tagFALSE
};


HINSTANCE g_hInstDbg = NULL;
HINSTANCE g_hInstLeak = NULL;

  
BOOL WINAPI DbgExEnableTag(TRACETAG tag, BOOL fEnable)
{
    BOOL fOld = FALSE;

    if (tag > 0 && tag < ARRAY_SIZE(g_rgtaginfo) - 1)
    {
        fOld = g_rgtaginfo[tag].fEnabled;
        g_rgtaginfo[tag].fEnabled = fEnable;
    }

    return(fOld);
}


char * GetModuleName(HINSTANCE hInst)
{
    static char achMod[MAX_PATH];
    achMod[0] = 0;
    GetModuleFileNameA(hInst, achMod, sizeof(achMod));
    char * psz = &achMod[lstrlenA(achMod)];
    while (psz > achMod && *psz != '\\' && *psz != '//') --psz;
    if (*psz == '\\' || *psz == '//') ++psz;
    return(psz);
}

void LeakDumpAppend(__in PSTR pszMsg, void * pvArg)
{
    HANDLE hFile;
    char ach[1024];
    char *pEnd = NULL;
    size_t iRemaining = 0;
    DWORD dw;

    StringCchCopyA(ach, ARRAY_SIZE(ach), GetModuleName(g_hInstLeak));
    StringCchCatExA(ach, ARRAY_SIZE(ach), ": ", &pEnd, &iRemaining, 0);
    StringCchVPrintfA(pEnd, iRemaining, pszMsg, (va_list)pvArg);

    hFile = CreateFileA("c:\\leakdump.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hFile, 0, NULL, FILE_END);
        WriteFile(hFile, ach, lstrlenA(ach), &dw, NULL);
        WriteFile(hFile, "\r\n", 2, &dw, NULL);
        CloseHandle(hFile);
    }
}

void WINAPI DbgExAddRefDebugLibrary()
{
}

void WINAPI DbgExReleaseDebugLibrary()
{
}

void WINAPI DbgExSetDllMain(HANDLE hDllHandle, BOOL (WINAPI * pfnDllMain)(HANDLE, DWORD, LPVOID))
{
}

void WINAPI DbgExDoTracePointsDialog(BOOL fWait)
{
}

void WINAPI DbgExRestoreDefaultDebugState()
{
}


TRACETAG WINAPI DbgExFindTag(__in PCSTR szTagDesc)
{
    struct TAGINFO * pti = g_rgtaginfo;
    TRACETAG tag;

    for (tag = 0; tag < ARRAY_SIZE(g_rgtaginfo); ++tag, ++pti)
    {
        if (!lstrcmpiA(pti->pchDesc, szTagDesc))
        {
            return(tag);
        }
    }

    return(local_tagFALSE);
}

TRACETAG WINAPI DbgExTagError()
{
    return(local_tagError);
}

TRACETAG WINAPI DbgExTagWarning()
{
    return(local_tagWarning);
}

TRACETAG WINAPI DbgExTagThread()
{
    return(local_tagThread);
}

TRACETAG WINAPI DbgExTagAssertExit()
{
    return(local_tagAssertExit);
}

TRACETAG WINAPI DbgExTagAssertStacks()
{
    return(local_tagAssertStacks);
}

TRACETAG WINAPI DbgExTagMemoryStrict()
{
    return(local_tagMemoryStrict);
}

TRACETAG WINAPI DbgExTagCoMemoryStrict()
{
    return(local_tagCoMemoryStrict);
}

TRACETAG WINAPI DbgExTagMemoryStrictTail()
{
    return(local_tagMemoryStrictTail);
}

TRACETAG WINAPI DbgExTagMemoryStrictAlign()
{
    return(local_tagMemoryStrictAlign);
}

TRACETAG WINAPI DbgExTagOLEWatch()
{
    return(local_tagOLEWatch);
}

TRACETAG WINAPI DbgExTagRegisterTrace(__in PCSTR szTag, __in PCSTR szOwner, __in PCSTR szDescrip, BOOL fEnabled)
{
    struct TAGINFO * pti = g_rgtaginfo;
    TRACETAG tag;

    for (tag = 0; tag < ARRAY_SIZE(g_rgtaginfo) - 1; ++tag, ++pti)
    {
        if (!lstrcmpiA(pti->pchDesc, szDescrip) && !lstrcmpiA(pti->pchOwner, szOwner))
        {
            return(tag);
        }
    }

    return(local_tagFALSE);
}

BOOL WINAPI DbgExIsTagEnabled(TRACETAG tag)
{
    return(tag >= 0 && tag < ARRAY_SIZE(g_rgtaginfo) && g_rgtaginfo[tag].fEnabled);
}


BOOL WINAPI DbgExTaggedTraceListEx(TRACETAG tag, USHORT usFlags, __in PCSTR szFmt, va_list valMarker)
{
    if (DbgExIsTagEnabled(tag))
    {
        CHAR    achDup[512], *pch;
        CHAR    achBuf[1024];
        CHAR    *pStart = achBuf;
        size_t  cch = ARRAY_SIZE(achBuf);
        achBuf[0] = '\0';

        StringCchCopyA(achDup, ARRAY_SIZE(achDup), szFmt);

        for (pch = achDup; *pch; ++pch)
        {
            if (*pch == '%')
            {
                if (pch[1] == '%')
                {
                    ++pch;
                    continue;
                }

                if (pch[1] == 'h' && pch[2] == 'r')
                {
                    pch[1] = 'l';
                    pch[2] = 'X';
                    continue;
                }
            }
        }

        if (!(usFlags & TAG_NONAME))
        {
            StringCchCopyExA(achBuf, ARRAY_SIZE(achBuf), "WPF: ", &pStart, &cch, 0);
        }

        StringCchVPrintfA(pStart, cch, szFmt, valMarker);

        if (!(usFlags & TAG_NONEWLINE))
        {
            StringCchCatA(achBuf, ARRAY_SIZE(achBuf), "\r\n");
        }

        OutputDebugStringA(achBuf);
    }

    return(FALSE);
}

void WINAPI DbgExTaggedTraceCallers(TRACETAG tag, int iStart, int cTotal)
{
}

void WINAPI DbgExAssertThreadDisable(BOOL fDisable)
{
}

size_t WINAPI DbgExPreAlloc(size_t cbRequest, PERFMETERTAG mt)
{
    return(cbRequest);
}

void * WINAPI DbgExPostAlloc(void *pv)
{
    return(pv);
}

void * WINAPI DbgExPreFree(void *pv)
{
    if (g_hInstDbg)
    {
        LeakDumpAppend("DbgExPreFree: freeing memory at %08lX", pv);
        pv = NULL;
    }

    return(pv);
}

void WINAPI DbgExPostFree()
{
}

size_t WINAPI DbgExPreRealloc(void *pvRequest, size_t cbRequest, void **ppv, PERFMETERTAG mt)
{
    *ppv = pvRequest;
    return(cbRequest);
}

void * WINAPI DbgExPostRealloc(void *pv)
{
    return(pv);
}

void * WINAPI DbgExPreGetSize(void *pvRequest)
{
    return(pvRequest);
}

size_t WINAPI DbgExPostGetSize(size_t cb)
{
    return(cb);
}

size_t WINAPI DbgExMtPreAlloc(size_t cbRequest, PERFMETERTAG mt)
{
    return(cbRequest);
}

void * WINAPI DbgExMtPostAlloc(void *pv)
{
    return(pv);
}

void * WINAPI DbgExMtPreFree(void *pv)
{
    if (g_hInstDbg)
    {
        LeakDumpAppend("DbgExMtPreFree: freeing memory at %08lX", pv);
        pv = NULL;
    }

    return(pv);
}

void WINAPI DbgExMtPostFree()
{
}

size_t WINAPI DbgExMtPreRealloc(void *pvRequest, size_t cbRequest, void **ppv, PERFMETERTAG mt)
{
    *ppv = pvRequest;
    return(cbRequest);
}

void * WINAPI DbgExMtPostRealloc(void *pv)
{
    return(pv);
}

void * WINAPI DbgExMtPreGetSize(void *pvRequest)
{
    return(pvRequest);
}

size_t WINAPI DbgExMtPostGetSize(size_t cb)
{
    return(cb);
}


void WINAPI DbgExMemoryTrackDisable(BOOL fDisable)
{
}

void WINAPI DbgExCoMemoryTrackDisable(BOOL fDisable)
{
}

void WINAPI DbgExMemoryBlockTrackDisable(void * pv)
{
}

void * WINAPI DbgExGetMallocSpy()
{
    return(NULL);
}

void WINAPI DbgExTraceMemoryLeaks()
{
}

BOOL WINAPI DbgExValidateKnownAllocations()
{
    return(TRUE);
}

LONG_PTR WINAPI DbgExTraceFailL(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    DPRINT1("Trace:\n pstrExpr: %s\n  pstrFile:  %s\n  line: %d\n   errTest: %p\n",
                pstrExpr, pstrFile, line, errTest);
    return(errExpr);
}

LONG_PTR WINAPI DbgExTraceWin32L(LONG_PTR errExpr, LONG_PTR errTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    DPRINT1("Trace:\n  pstrExpr: %s\n  pstrFile:  %s\n  line: %d\n    errTest: %p\n",
                 pstrExpr, pstrFile, line, errTest);
    return(errExpr);
}

HRESULT WINAPI DbgExTraceHR(HRESULT hrTest, BOOL fIgnore, __in PCSTR pstrExpr, __in PCSTR pstrFile, int line)
{
    DPRINT1("Trace\n pstrExpr: %s\n  pstrFile:  %s\n  line: %d\n  HRESULT: %X\n",
                pstrExpr, pstrFile, line, hrTest);
    if (hrTest != S_OK)
        __debugbreak();
    return(hrTest);
}

HRESULT WINAPI _DbgExTraceOLE(HRESULT hrTest, BOOL fIgnore, __in LPSTR pstrExpr, __in LPSTR pstrFile, int line, LPVOID lpsite)
{
    DPRINT1("Trace:\n pstrExpr: %s\n  pstrFile:  %s\n  line: %d\n",
            pstrExpr, pstrFile, line);
    return(hrTest);
}

void WINAPI DbgExSetSimFailCounts(int firstFailure, int cInterval)
{
}

void WINAPI DbgExShowSimFailDlg()
{
}

BOOL WINAPI DbgExFFail()
{
    return(FALSE);
}

int WINAPI DbgExGetFailCount()
{
    return(INT_MIN);
}

void WINAPI DbgExOpenMemoryMonitor()
{
}

void WINAPI DbgExOpenLogFile(LPCSTR szFName)
{
}

void WINAPI DbgExDumpProcessHeaps()
{
} 

PERFMETERTAG WINAPI DbgExMtRegister(__in PCSTR szTag, __in PCSTR szOwner, __in PCSTR szDescrip, DWORD dwFlags)
{
    return(0);
}

void WINAPI DbgExMtAdd(PERFMETERTAG mt, LONG lCnt, LONG lVal)
{
}

void WINAPI DbgExMtSet(PERFMETERTAG mt, LONG lCnt, LONG lVal)
{
}

char * WINAPI DbgExMtGetName(PERFMETERTAG mt)
{
    return("");
}

char * WINAPI DbgExMtGetDesc(PERFMETERTAG mt)
{
    return("");
}

PERFMETERTAG WINAPI DbgExMtGetParent(PERFMETERTAG mt)
{
    return (PERFMETERTAG)NULL;
}

DWORD WINAPI DbgExMtGetFlags(PERFMETERTAG mt)
{
    return 0;
}

void WINAPI DbgExMtSetFlags(PERFMETERTAG mt, DWORD dwFlags)
{
    return;
}

BOOL WINAPI DbgExMtSimulateOutOfMemory(PERFMETERTAG mt, LONG lNewValue)
{
    return(0);
}

void WINAPI DbgExMtOpenMonitor()
{
}

void WINAPI DbgExMtLogDump(__in PCSTR pchFile)
{
}

PERFMETERTAG WINAPI DbgExMtLookupMeter(__in PCSTR szTag)
{
    return 0;
}

PERFMETERTAG WINAPI bgExMtLookupMeter(__in PCSTR szTag)
{
    return 0;
}

long WINAPI DbgExMtGetMeterCnt(PERFMETERTAG mt, BOOL fExclusive)
{
    return 0;
}

long WINAPI DbgExMtGetMeterVal(PERFMETERTAG mt, BOOL fExclusive)
{
    return 0;
}

PERFMETERTAG WINAPI DbgExMtGetDefaultMeter()
{
    return (PERFMETERTAG)NULL;
}

PERFMETERTAG WINAPI DbgExMtSetDefaultMeter(PERFMETERTAG mtDefault)
{
    return (PERFMETERTAG)NULL;
}

void WINAPI DbgExSetTopUrl(__in LPWSTR pstrUrl)
{
}

void WINAPI DbgExGetSymbolFromAddress(void * pvAddr,  __out_ecount(cchBuf) char * pszBuf, DWORD cchBuf)
{
    pszBuf[0] = 0;
}

void WINAPI DbgExGetStackAddresses(void ** ppvAddr, int iStart, int cTotal)
{
    memset( ppvAddr, 0, sizeof(void *) * cTotal );
}


BOOL WINAPI DbgExGetChkStkFill(DWORD * pdwFill)
{
    *pdwFill = GetPrivateProfileIntA("chkstk", "fill", 0xCCCCCCCC, "avalndbg.ini");
    return(!GetPrivateProfileIntA("chkstk", "disable", FALSE, "avalndbg.ini"));
}
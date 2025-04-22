#include "PresentationDebug.h"


#define ARRAY_SIZE(x)           (sizeof(x) / sizeof(x[0]))
DWORD WINAPI DbgExGetVersion()
{
    return 2;
}

BOOL WINAPI DbgExIsFullDebug()
{
    return TRUE;
}

BOOL WINAPI DbgExSetDiskFlag(TRACETAG tag, BOOL fSendToDisk)
{
    return FALSE;
}

BOOL WINAPI DbgExSetBreakFlag(TRACETAG tag, BOOL fBreak)
{
    return FALSE;
}

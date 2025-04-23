/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * PURPOSE:
 * COPYRIGHT:       Copyright 2021 Justin Miller (justinmiller100@gmail.com)
 */

#pragma once

/* INCLUDES ******************************************************************/

#include <stdio.h>
#include <stdlib.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnls.h>
#include <winreg.h>
#include <winnls32.h>
#include <winver.h>

#define NTOS_MODE_USER
#include <ndk/umtypes.h>
#include <ndk/pstypes.h>
#include <ndk/rtlfuncs.h>

struct TAGINFO
{
    CHAR *  pchOwner;
    CHAR *  pchDesc;
    BOOL    fEnabled;
};

typedef INT     TRACETAG;
typedef INT_PTR PERFTAG;
typedef INT_PTR PERFMETERTAG;


 #define TraceTag(x)                         \
        do                                      \
        {                                       \
            if (DbgExTaggedTrace x) {                \
                AvalonDebugBreak();             \
            }                                   \
        } while (UNCONDITIONAL_EXPR(false))

    #define TraceTagEx(x)                       \
        do                                      \
        {                                       \
            if (DbgExTaggedTraceEx x) {              \
                AvalonDebugBreak();             \
            }                                   \
        } while (UNCONDITIONAL_EXPR(false))

    #define TraceCallers(tag, iStart, cTotal)   \
        DbgExTaggedTraceCallers(tag, iStart, cTotal)

    #define DeclareTag(tag, szOwner, szDescrip) \
        TRACETAG tag(DbgExTagRegisterTrace(#tag, szOwner, szDescrip, FALSE));
    #define DeclareTagEx(tag, szOwner, szDescrip, fEnabled) \
        TRACETAG tag(DbgExTagRegisterTrace(#tag, szOwner, szDescrip, fEnabled));
    #define ExternTag(tag) extern TRACETAG tag;

    #define PerfDbgTag(tag, szOwner, szDescrip) DeclareTag(tag, szOwner, szDescrip)
    #define PerfDbgTagOther(tag, szOwner, szDescrip) DeclareTagOther(tag, szOwner, szDescrip)
    #define PerfDbgExtern(tag) ExternTag(tag)
    #define IsPerfDbgEnabled(tag) IsTagEnabled(tag)

    // DbgExTaggedTraceEx usFlags parameter defines
    #define TAG_NONAME      0x01
    #define TAG_NONEWLINE   0x02
    #define TAG_USECONSOLE  0x04
    #define TAG_INDENT      0x08
    #define TAG_OUTDENT     0x10

    // Standard tags
    #define tagError                DbgExTagError()
    #define tagWarning              DbgExTagWarning()
    #define tagThread               DbgExTagThread()
    #define tagAssertExit           DbgExTagAssertExit()
    #define tagAssertStacks         DbgExTagAssertStacks()
    #define tagMemoryStrict         DbgExTagMemoryStrict()
    #define tagCoMemoryStrict       DbgExTagCoMemoryStrict()
    #define tagMemoryStrictTail     DbgExTagMemoryStrictTail()
    #define tagMemoryStrictAlign    DbgExTagMemoryStrictAlign()
    #define tagOLEWatch             DbgExTagOLEWatch()
    
    // Get/Set tag enabled status.
    #define IsTagEnabled            DbgExIsTagEnabled
    #define EnableTag               DbgExEnableTag
    #define SetDiskFlag             DbgExSetDiskFlag
    #define SetBreakFlag            DbgExSetBreakFlag
    #define FindTag                 DbgExFindTag
  
  
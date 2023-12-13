
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            dll/win32/kernel32/k32.h
 * PURPOSE:         Win32 Kernel Library Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

#ifndef __KB_H
#define __KB_H

/* INCLUDES ******************************************************************/

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <wincon.h>
#include <winuser.h>

/* Re#define NTDDI_VERSION to 2K3 SP1 to get correct NDK definitions */
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1


#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/iofuncs.h>
#include <ndk/kdtypes.h>
#include <ndk/kefuncs.h>
#include <ndk/ldrfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>
#include <ndk/umfuncs.h>

typedef HANDLE EVT_HANDLE, *PEVT_HANDLE;

typedef enum _EVT_VARIANT_TYPE
{
    EvtVarTypeNull        = 0,
    EvtVarTypeString      = 1,
    EvtVarTypeAnsiString  = 2,
    EvtVarTypeSByte       = 3,
    EvtVarTypeByte        = 4,
    EvtVarTypeInt16       = 5,
    EvtVarTypeUInt16      = 6,
    EvtVarTypeInt32       = 7,
    EvtVarTypeUInt32      = 8,
    EvtVarTypeInt64       = 9,
    EvtVarTypeUInt64      = 10,
    EvtVarTypeSingle      = 11,
    EvtVarTypeDouble      = 12,
    EvtVarTypeBoolean     = 13,
    EvtVarTypeBinary      = 14,
    EvtVarTypeGuid        = 15,
    EvtVarTypeSizeT       = 16,
    EvtVarTypeFileTime    = 17,
    EvtVarTypeSysTime     = 18,
    EvtVarTypeSid         = 19,
    EvtVarTypeHexInt32    = 20,
    EvtVarTypeHexInt64    = 21,

    // these types used internally
    EvtVarTypeEvtHandle   = 32,
    EvtVarTypeEvtXml      = 35

} EVT_VARIANT_TYPE;


typedef enum _EVT_CHANNEL_CONFIG_PROPERTY_ID
{
    EvtChannelConfigEnabled = 0,            // EvtVarTypeBoolean
    EvtChannelConfigIsolation,              // EvtVarTypeUInt32, EVT_CHANNEL_ISOLATION_TYPE
    EvtChannelConfigType,                   // EvtVarTypeUInt32, EVT_CHANNEL_TYPE
    EvtChannelConfigOwningPublisher,        // EvtVarTypeString
    EvtChannelConfigClassicEventlog,        // EvtVarTypeBoolean
    EvtChannelConfigAccess,                 // EvtVarTypeString
    EvtChannelLoggingConfigRetention,       // EvtVarTypeBoolean
    EvtChannelLoggingConfigAutoBackup,      // EvtVarTypeBoolean
    EvtChannelLoggingConfigMaxSize,         // EvtVarTypeUInt64
    EvtChannelLoggingConfigLogFilePath,     // EvtVarTypeString
    EvtChannelPublishingConfigLevel,        // EvtVarTypeUInt32
    EvtChannelPublishingConfigKeywords,     // EvtVarTypeUInt64
    EvtChannelPublishingConfigControlGuid,  // EvtVarTypeGuid
    EvtChannelPublishingConfigBufferSize,   // EvtVarTypeUInt32
    EvtChannelPublishingConfigMinBuffers,   // EvtVarTypeUInt32
    EvtChannelPublishingConfigMaxBuffers,   // EvtVarTypeUInt32
    EvtChannelPublishingConfigLatency,      // EvtVarTypeUInt32
    EvtChannelPublishingConfigClockType,    // EvtVarTypeUInt32, EVT_CHANNEL_CLOCK_TYPE
    EvtChannelPublishingConfigSidType,      // EvtVarTypeUInt32, EVT_CHANNEL_SID_TYPE
    EvtChannelPublisherList,                // EvtVarTypeString | EVT_VARIANT_TYPE_ARRAY
    EvtChannelPublishingConfigFileMax,      // EvtVarTypeUint32
    EvtChannelConfigPropertyIdEND

} EVT_CHANNEL_CONFIG_PROPERTY_ID;

typedef enum _EVT_CHANNEL_TYPE
{
    EvtChannelTypeAdmin = 0,
    EvtChannelTypeOperational,
    EvtChannelTypeAnalytic,
    EvtChannelTypeDebug

} EVT_CHANNEL_TYPE;

typedef enum _EVT_CHANNEL_ISOLATION_TYPE
{
    EvtChannelIsolationTypeApplication = 0,
    EvtChannelIsolationTypeSystem,
    EvtChannelIsolationTypeCustom

} EVT_CHANNEL_ISOLATION_TYPE;

typedef enum _EVT_CHANNEL_CLOCK_TYPE
{
    EvtChannelClockTypeSystemTime = 0,      // System time
    EvtChannelClockTypeQPC                  // Query performance counter

} EVT_CHANNEL_CLOCK_TYPE;

typedef enum _EVT_CHANNEL_SID_TYPE
{
    EvtChannelSidTypeNone = 0,
    EvtChannelSidTypePublishing

} EVT_CHANNEL_SID_TYPE;

typedef struct _EVT_VARIANT
{
    union
    {
        BOOL        BooleanVal;
        INT8        SByteVal;
        INT16       Int16Val;
        INT32       Int32Val;
        INT64       Int64Val;
        UINT8       ByteVal;
        UINT16      UInt16Val;
        UINT32      UInt32Val;
        UINT64      UInt64Val;
        float       SingleVal;
        double      DoubleVal;
        ULONGLONG   FileTimeVal;
        SYSTEMTIME* SysTimeVal;
        GUID*       GuidVal;
        LPCWSTR     StringVal;
        LPCSTR      AnsiStringVal;
        PBYTE       BinaryVal;
        PSID        SidVal;
        size_t      SizeTVal;

        // array fields
        BOOL*       BooleanArr;
        INT8*       SByteArr;
        INT16*      Int16Arr;
        INT32*      Int32Arr;
        INT64*      Int64Arr;
        UINT8*      ByteArr;
        UINT16*     UInt16Arr;
        UINT32*     UInt32Arr;
        UINT64*     UInt64Arr;
        float*      SingleArr;
        double*     DoubleArr;
        FILETIME*   FileTimeArr;
        SYSTEMTIME* SysTimeArr;
        GUID*       GuidArr;
        LPWSTR*     StringArr;
        LPSTR*      AnsiStringArr;
        PSID*       SidArr;
        size_t*     SizeTArr;

        // internal fields
        EVT_HANDLE  EvtHandleVal;
        LPCWSTR     XmlVal;
        LPCWSTR*    XmlValArr;
    };

    DWORD Count;   // for arrays: number of elements (not the number of bytes).
    DWORD Type;

} EVT_VARIANT, *PEVT_VARIANT;
typedef enum _EVT_SUBSCRIBE_NOTIFY_ACTION
{
    EvtSubscribeActionError = 0,
    EvtSubscribeActionDeliver

} EVT_SUBSCRIBE_NOTIFY_ACTION;

typedef enum _EVT_LOGIN_CLASS
{
    EvtRpcLogin = 1

} EVT_LOGIN_CLASS;

typedef DWORD (WINAPI *EVT_SUBSCRIBE_CALLBACK)(
    EVT_SUBSCRIBE_NOTIFY_ACTION Action,
    _Maybenull_ PVOID UserContext,
    _In_ EVT_HANDLE Event );
#endif /* __KB_H */
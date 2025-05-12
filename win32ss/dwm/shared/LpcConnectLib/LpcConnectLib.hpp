#pragma once

/* PSDK/NDK Headers */
#include <stdio.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#define NTOS_MODE_USER
#include <ndk/obtypes.h>
#include <ndk/obfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/lpcfuncs.h>
#include <ndk/umfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <ndk/kefuncs.h>

#define MAX_MESSAGE_DATA   (0x130)
typedef struct _LPC_MAX_MESSAGE
{
    PORT_MESSAGE Header;
    ULONG Message;
    ULONG Status;
    BYTE Data[MAX_MESSAGE_DATA - sizeof(ULONG) - sizeof(ULONG)];
} LPC_MAX_MESSAGE, *PLPC_MAX_MESSAGE;


class LpcConnectLib
{
private:
    HANDLE InstancePort;

public:
    LpcConnectLib();
    ~LpcConnectLib();

    NTSTATUS ConnectToPortHandle(HANDLE PortHandle);
    NTSTATUS ConnectToPortString(PCWSTR SourceString,
                                 PCWSTR SourceDesc);
    NTSTATUS DisconnectFromPort();

    NTSTATUS SendComplexAsyncRequest(UINT32 Command,
                                     LPVOID InData,
                                     SIZE_T InSize); //SendComplexAsyncRequestNative

    NTSTATUS SendComplexSyncRequest(UINT32 Command,
                                    LPVOID InData,
                                    SIZE_T InSize,
                                    LPVOID OutData,
                                    SIZE_T OutSize,
                                    HRESULT* Request); //SendComplexSyncRequestNative

    NTSTATUS SendSimpleAsyncRequest(UINT32 Command); //SendSimpleAsyncRequestNative
  //  NTSTATUS SendSimpleSyncRequest //SendSimpleSyncRequestNative
};

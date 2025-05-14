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


#include <dwmlpc.h>

typedef
NTSTATUS
INTERNALLPCHANDLER(PLPC_MAX_MESSAGE LpcReply, PVOID PortContext);

typedef INTERNALLPCHANDLER *PINTERNALLPCHANDLER;

class LpcCreateLib
{
private:
public:
    HANDLE InstancePort;
    HANDLE GlobalGenericThread;
    PINTERNALLPCHANDLER LpcHandler;
    LpcCreateLib();
    ~LpcCreateLib();

    NTSTATUS StartPortThread(HANDLE hSourceHandle);
    NTSTATUS StopPortThread();
    NTSTATUS WaitOnPortThread();
    VOID WINAPI ProcessLpcOperation(PLPC_MAX_MESSAGE LpcReply, PVOID PortContext);
    DWORD WINAPI ProcessCompleteConnect(PLPC_MAX_MESSAGE LpcInput);
};

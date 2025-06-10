/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32/src/sputil.c
 * PURPOSE:     Transport Service Provider Utility Functions
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
INT
WSPAPI
WPUCompleteOverlappedRequest(IN SOCKET s,
                             IN LPWSAOVERLAPPED lpOverlapped,
                             IN DWORD dwError,
                             IN DWORD cbTransferred,
                             OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WSPAPI
WPUCloseEvent(IN WSAEVENT hEvent,
              OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WPUCloseThread(IN LPWSATHREADID lpThreadId,
               OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
WSAEVENT
WSPAPI
WPUCreateEvent(OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return (WSAEVENT)0;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WPUOpenCurrentThread(OUT LPWSATHREADID lpThreadId,
                     OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
BOOL
WSPAPI
WPUPostMessage(IN HWND hWnd,
               IN UINT Msg,
               IN WPARAM wParam,
               IN LPARAM lParam)
{
    /* Make sure we have a post routine */
    if (!WsSockPostRoutine) WsSockPostRoutine = PostMessage;

    /* Call it */
    return WsSockPostRoutine(hWnd, Msg, wParam, lParam);
}

/*
 * @implemented
 */
INT
WSPAPI
WPUQueryBlockingCallback(IN DWORD dwCatalogEntryId,
                         OUT LPBLOCKINGCALLBACK FAR* lplpfnCallback,
                         OUT PDWORD_PTR lpdwContext,
                         OUT LPINT lpErrno)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PTCATALOG Catalog;
    INT ErrorCode;
    INT Status;
    LPBLOCKINGCALLBACK Callback = NULL;
    PTCATALOG_ENTRY Entry;
    DWORD_PTR Context = 0;
    DPRINT("WPUQueryBlockingCallback: %lx \n", dwCatalogEntryId);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Get the callback function */
        Callback = Thread->BlockingCallback;

        /* Check if there is one */
        if (Callback)
        {
            /* Get the catalog */
            Catalog = WsProcGetTCatalog(Process);

            /* Find the entry for this ID */
            ErrorCode = WsTcGetEntryFromCatalogEntryId(Catalog,
                                                       dwCatalogEntryId,
                                                       &Entry);

            /* Check for success */
            if (ErrorCode == ERROR_SUCCESS)
            {
                /* Get the context */
                Context = (DWORD_PTR)Entry->Provider->Service.lpWSPCancelBlockingCall;

                /* Dereference the entry */
                WsTcEntryDereference(Entry);
            }
        }
    }

    /* Check error code */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Return success as well */
        Status = ERROR_SUCCESS;
    }
    else
    {
        /* Return expected value and no callback */
        Status = SOCKET_ERROR;
        Callback = NULL;
    }

    /* Return the settings */
    *lpdwContext = Context;
    *lpErrno = ErrorCode;
    *lplpfnCallback = Callback;

    /* Return to caller */
    return Status;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WPUQueueApc(IN LPWSATHREADID lpThreadId,
            IN LPWSAUSERAPC lpfnUserApc,
            IN DWORD_PTR dwContext,
            OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WSPAPI
WPUResetEvent(IN WSAEVENT hEvent,
              OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WSPAPI
WPUSetEvent(IN WSAEVENT hEvent,
            OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return FALSE;
}

typedef enum _WSC_PROVIDER_INFO_TYPE {
//  InfoType is:                  Info points to:
    ProviderInfoLspCategories, // DWORD (LspCategories)
    ProviderInfoAudit,         // struct WSC_PROVIDER_AUDIT_INFO
} WSC_PROVIDER_INFO_TYPE ;

int WINAPI WSCGetProviderInfo(
    LPGUID                 lpProviderId,
    WSC_PROVIDER_INFO_TYPE InfoType,
    PBYTE                  Info,
    size_t                 *InfoSize,
    DWORD                  Flags,
    LPINT                  lpErrno
)
{
    if (!lpProviderId || !Info || !InfoSize || !lpErrno)
    {
        if (lpErrno) *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    // This is a stub implementation, as ReactOS does not yet support LSP categories or provider audit info.
    switch (InfoType)
    {
        case ProviderInfoLspCategories:
            if (*InfoSize < sizeof(DWORD))
            {
                *InfoSize = sizeof(DWORD);
                *lpErrno = WSAEFAULT;
                return SOCKET_ERROR;
            }
            *(DWORD*)Info = 0; // No LSP categories
            *InfoSize = sizeof(DWORD);
            *lpErrno = 0;
            return 0;

        case ProviderInfoAudit:
            *lpErrno = WSAEOPNOTSUPP;
            return SOCKET_ERROR;

        default:
            *lpErrno = WSAEINVAL;
            return SOCKET_ERROR;
    }
    return 0;
}

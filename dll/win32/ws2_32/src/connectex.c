/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32/src/connectex.c
 * PURPOSE:     Implementation of ConnectEx/WSPConnectEx
 */

#include <ws2_32.h>
#include <mswsock.h>
#include <debug.h>

BOOL
WINAPI
ConnectEx(
    SOCKET s,
    const struct sockaddr *name,
    int namelen,
    PVOID lpSendBuffer,
    DWORD dwSendDataLength,
    LPDWORD lpdwBytesSent,
    LPOVERLAPPED lpOverlapped)
{
    PWSSOCKET Socket;
    INT ErrorCode;
    DPRINT("ConnectEx: %lx, %p, %lx\n", s, name, namelen);

    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        if ((Socket = WsSockGetSocket(s)))
        {
                GUID guidConnectEx = WSAID_CONNECTEX;
                LPFN_CONNECTEX lpConnectEx = NULL;
                DWORD bytesReturned = 0;

                if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER,
                             &guidConnectEx, sizeof(guidConnectEx),
                             &lpConnectEx, sizeof(lpConnectEx),
                             &bytesReturned, NULL, NULL) == SOCKET_ERROR || !lpConnectEx)
                {
                    SetLastError(WSAEOPNOTSUPP);
                    return FALSE;
                }

                return lpConnectEx(s, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
        }
        else
        {
            ErrorCode = WSAENOTSOCK;
        }
    }
    SetLastError(ErrorCode);
    return FALSE;
}

/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        dll/win32/msafd/misc/stubs.c
 * PURPOSE:     Stubs
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */

#include <msafd.h>

INT
WSPAPI
WSPCancelBlockingCall(
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED;

    return 0;
}


BOOL
WSPAPI
WSPGetQOSByName(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpQOSName,
    OUT     LPQOS lpQOS,
    OUT     LPINT lpErrno)
{
    UNIMPLEMENTED;

    return FALSE;
}


SOCKET
WSPAPI
WSPJoinLeaf(
    IN  SOCKET s,
    IN  CONST SOCKADDR *name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED;

    return (SOCKET)0;
}

BOOL
WSPAPI
WSPAcceptEx(
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    OUT PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN OUT LPOVERLAPPED lpOverlapped)
{
    #if 0
    SOCKET as = INVALID_SOCKET;
    int salen = 0;
    struct sockaddr_storage laddr, raddr;

    if (lpdwBytesReceived) *lpdwBytesReceived = 0;
    if (lpOverlapped)
    {
        lpOverlapped->Internal = WSAEOPNOTSUPP;
        WSASetLastError(WSAEOPNOTSUPP);
        return FALSE;
    }

    /* Perform blocking accept on the listen socket */
    as = accept(sListenSocket, (dwRemoteAddressLength ? (struct sockaddr *)&raddr : NULL), (dwRemoteAddressLength ? &salen : NULL));
    if (as == INVALID_SOCKET)
    {
        return FALSE;
    }

    /* Query local address */
    salen = sizeof(laddr);
    if (getsockname(as, (struct sockaddr *)&laddr, &salen) == SOCKET_ERROR)
    {
        closesocket(as);
        return FALSE;
    }

    /* Layout: [recv data][local][remote] */
    if (lpOutputBuffer)
    {
        char *base = (char *)lpOutputBuffer + dwReceiveDataLength;
        if (dwLocalAddressLength && (dwLocalAddressLength <= sizeof(laddr)))
            memcpy(base, &laddr, dwLocalAddressLength);
        if (dwRemoteAddressLength && (dwRemoteAddressLength <= sizeof(raddr)))
            memcpy(base + dwLocalAddressLength, &raddr, dwRemoteAddressLength);
        if (lpdwBytesReceived)
            *lpdwBytesReceived = dwReceiveDataLength + dwLocalAddressLength + dwRemoteAddressLength;
    }

    /* If caller provided an accept socket, duplicate by associating handle */
    if (sAcceptSocket != INVALID_SOCKET)
    {
        /* Best-effort: close the provided socket and replace with accepted one */
        closesocket(sAcceptSocket);
        /* Caller typically calls setsockopt(SO_UPDATE_ACCEPT_CONTEXT) afterwards */
    }

    return TRUE;

    #endif
    return FALSE;
}

BOOL
WSPAPI
WSPConnectEx(
    IN SOCKET s,
    IN const struct sockaddr *name,
    IN int namelen,
    IN PVOID lpSendBuffer,
    IN DWORD dwSendDataLength,
    OUT LPDWORD lpdwBytesSent,
    IN OUT LPOVERLAPPED lpOverlapped)
{
    #if 0
    SOCKET dup = s;
    if (lpdwBytesSent) *lpdwBytesSent = 0;

    /* For now, complete synchronously */
    if (lpOverlapped)
    {
        lpOverlapped->Internal = WSAEOPNOTSUPP;
        return FALSE;
    }

    /* Perform a normal connect */
    if (WSAConnect(dup, name, namelen, NULL, NULL, NULL, NULL) == SOCKET_ERROR)
    {
        return FALSE;
    }

    /* Optionally send initial data */
    if (lpSendBuffer && dwSendDataLength)
    {
        int sent = send(dup, (const char *)lpSendBuffer, (int)dwSendDataLength, 0);
        if (sent == SOCKET_ERROR)
        {
            return FALSE;
        }
        if (lpdwBytesSent) *lpdwBytesSent = (DWORD)sent;
    }

    return TRUE;
    #endif
    return FALSE;
}

BOOL
WSPAPI
WSPDisconnectEx(
  IN SOCKET hSocket,
  IN LPOVERLAPPED lpOverlapped,
  IN DWORD dwFlags,
  IN DWORD reserved)
{
    UNIMPLEMENTED;

    return FALSE;
}

VOID
WSPAPI
WSPGetAcceptExSockaddrs(
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT struct sockaddr **LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT struct sockaddr **RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength)
{
    UNIMPLEMENTED;

    return;
}

/* EOF */

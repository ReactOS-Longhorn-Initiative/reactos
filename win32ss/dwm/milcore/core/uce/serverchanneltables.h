// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
// 

// 
//  Abstract:
//      Definitions for client and server-side channel handle tables.
// 
//------------------------------------------------------------------------------

#define DEVICE_ENTRY 1
#define DEVICE_ENTRY_MASTER 2
#define DEVICE_ENTRY_SLAVE 3

interface IMilTransportQueue;
class CMilSlaveHandleTable;
class CComposition;
class CMilServerChannel;

struct SERVER_CHANNEL_HANDLE_ENTRY
{
    DWORD type;
    HMIL_CHANNEL hSourceChannel;
    CMilServerChannel *pServerChannel;
    CComposition *pCompDevice;
};

class CMilServerChannelTable : public HANDLE_TABLE
{
public:
    CMilServerChannelTable(UINT cbEntry);
    ~CMilServerChannelTable();

    HRESULT AssignChannelEntry(
        _In_ HMIL_CHANNEL hChannel
        );

    HRESULT GetServerChannelTableEntry(
        _In_ HMIL_CHANNEL hChannel,
        __out_ecount(1) SERVER_CHANNEL_HANDLE_ENTRY **ppMasterEntry
        );

    VOID DestroyHandle(
        _In_ HMIL_CHANNEL object
        );

    HRESULT GetServerChannel(
        _In_ HMIL_CHANNEL hChannel,
        __out_ecount(1) CMilServerChannel **ppServerChannel
        );

private:
    HRESULT GetSlaveTableEntry(
        _In_ HMIL_CHANNEL hChannel,
        __out_ecount(1) SERVER_CHANNEL_HANDLE_ENTRY **ppMasterEntry
        );
};


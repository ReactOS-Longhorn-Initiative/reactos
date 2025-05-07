// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Definition for the cross packet transport channels.
//
//------------------------------------------------------------------------------

MtExtern(CMilServerChannel);

class CMilSlaveHandleTable;
class CConnectionContext;
interface IMilBatchDevice;

class CMilServerChannel : 
    public CMILRefCountBase
{
protected:
    CMilServerChannel(
        _In_ CConnectionContext *pTransport,
        _In_ IMilBatchDevice *pdevTarget,
        HMIL_CHANNEL hChannel
        );

    virtual ~CMilServerChannel();

public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilServerChannel));
    DEFINE_REF_COUNT_BASE

    static HRESULT Create(
        _In_ CConnectionContext *pTransport,
        _In_ IMilBatchDevice *pdevTarget,
        HMIL_CHANNEL hChannel,
        _Out_ CMilServerChannel **ppChannel
        );


    HRESULT PostMessageToChannel(
        __in_ecount(1) const MIL_MESSAGE *pLocalNotification
        );

    HRESULT SubmitBatch(_In_ CMilCommandBatch* pBatch);
    CMilSlaveHandleTable* GetChannelTable();

    HMIL_CHANNEL GetChannel() const
    {
        return m_hChannel;
    }

    // Returns the composition device associated with this channel.
    CComposition *GetComposition();

    VOID SignalFinishedFlush(HRESULT hrReported);
    VOID SetServerSideFlushEvent(HANDLE hSyncFlushEvent);

private:

    // These the channel handle and pid the channel uses to post notifications back.
    HMIL_CHANNEL m_hChannel;

    // this points to the server side composition device where batches need to
    // be enqueued. It is the entry point used to enqueue batches into the change queue.
    IMilBatchDevice *m_pDevice;

    // Spin slave tables for each channel.
    CMilSlaveHandleTable *m_pServerTable;

    // This points to the transport object used by the channel to post notifications. 
    CConnectionContext *m_pTransport;

    HANDLE m_hSyncFlushEvent;
};



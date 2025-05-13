// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

MtExtern(UpdateState);

//
// UpdateState is a CStateThreadItem whose Run method updates CWmpStateEngine.
// WmpPlayer sets the desired state and calls CWmpStateEngine::AddItem so
// that UpdateState::Run is called from the apartment thread.
//
class UpdateState : public CStateThreadItem,
                    public CMILCOMBase
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(UpdateState));

    DECLARE_COM_BASE;

    static
    HRESULT
    Create(
        _In_    MediaInstance       *pMediaInstance,
        _In_    CWmpStateEngine     *pCWmpStateEngine,
        _In_    UpdateState         **ppUpdateState
        );

    void
    OpenHelper(
        _In_ LPCWSTR pwszURL
        );

    void
    SetRateHelper(
        _In_ double dRate
        );

    void
    SetTargetActionState(
        _In_    ActionState::Enum   targetActionState
        );

    void
    SetTargetVolume(
        _In_    long                targetVolume
        );

    void
    SetTargetBalance(
        _In_    long                targetBalance
        );

    void
    SetTargetSeekTo(
        _In_    double              targetSeekTo
        );

    void
    SetTargetIsScrubbingEnabled(
        _In_    bool                isScrubbingEnabled
        );

    void
    UpdateTransients(
        void
        );

    void
    Close(
        void
        );

    HRESULT
    UpdateTransientsSync(
        _In_    DWORD       timeOutInMilliseconds,
        _Out_   bool        *pDidTimeOut
        );

protected:
    //
    // CMILCOMBase
    //
    STDMETHOD(HrFindInterface)(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppvObject
        );

    //
    // CStateThreadItem
    //
    /* override */
    void
    Run(
        void
        );

private:
    UpdateState(
        _In_    MediaInstance   *pMediaInstance,
        _In_    CWmpStateEngine *pCWmpStateEngine
        );

    ~UpdateState(
        void
        );

    HRESULT
    Init(
        void
        );

    CCriticalSection            m_lock;
    UINT                        m_uiID;
    MediaInstance               *m_pMediaInstance;
    CWmpStateEngine             *m_pCWmpStateEngine;
    HANDLE                      m_waitEvent;

    Optional<ActionState::Enum> m_targetActionState;
    Optional<bool>              m_targetOcx;
    Optional<double>            m_targetRate;
    Optional<PWSTR>             m_targetUrl;
    Optional<long>              m_targetVolume;
    Optional<long>              m_targetBalance;
    Optional<double>            m_targetSeekTo;
    Optional<bool>              m_targetIsScrubbingEnabled;

    bool                        m_didUrlChange;
    bool                        m_doUpdateTransients;
    bool                        m_doClose;
    LONGLONG                    m_lastRequest;
    LONGLONG                    m_lastUpdate;
};


// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_media
//      $Keywords:
//
//  $Description:
//      Provides a framework to call methods in the state thread and wait for
//      them to complete.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#pragma once

MtExtern(WmpStateEngineProxyItem);

template <class Class, typename Datatype>
class WmpStateEngineProxyItem : public CStateThreadItem,
                                public CMILCOMBase
{
public:
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(WmpStateEngineProxyItem));

    typedef
    HRESULT
    (Class::*Method)(
        Datatype
        );

    // Declares IUnknown functions
    DECLARE_COM_BASE;

    inline
    WmpStateEngineProxyItem(
        _In_    UINT            uiID,
        _In_    CWmpStateEngine *pCWmpStateEngine,
        _In_    Class           *pClass,
        _In_    Method          method,
                Datatype        data
        );

    inline
    ~WmpStateEngineProxyItem(
        void
        );

    inline
    HRESULT
    CallMethod(
        _In_    bool            waitForCompletion
        );

protected:

    //
    // CMILCOMBase
    //
    STDMETHOD(HrFindInterface)(
        _In_ REFIID riid,
        __deref_out void **ppv
        );

    /* override */
    void
    Run(
        void
       );

    /* override */
    void
    Cancel(
        void
        );

    /* override */
    bool
    IsAnOwner(
        _In_    IUnknown    *pIUnknown
        );

private:

    UINT            m_uiID;
    CWmpStateEngine *m_pCWmpStateEngine;
    Class           *m_pClass;
    Method          m_method;
    Datatype        m_data;
    HANDLE          m_callCompletedEvent;
    HRESULT         m_result;
};

namespace WmpStateEngineProxy
{

template <typename Class, typename Method, typename Datatype>
inline
HRESULT
CallMethod(
    _In_        UINT                uiID,
    _In_        CWmpStateEngine     *pCWmpStateEngine,
    _In_        Class               *pClass,
    _In_        Method              method,
                Datatype            data
    );

template <typename Class, typename Method, typename Datatype>
HRESULT
AsyncCallMethod(
    _In_        UINT                uiID,
    _In_        CWmpStateEngine     *pCWmpStateEngine,
    _In_        Class               *pClass,
    _In_        Method              method,
                Datatype            data
    );
};

#include "WmpStateEngineProxy.inl"


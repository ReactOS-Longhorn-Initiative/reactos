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
//      Header for CMILAV, which facilitates calling unmanaged code from managed
//      code.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#pragma once

class MediaInstance;
class CMediaEventProxy;
class CEventProxy;

class CMILAV
{
public:

    static HRESULT CreateMedia(
        _In_        CEventProxy *pEventProxy,
        _In_        bool        canOpenAnyMedia,
        __deref_out IMILMedia   **ppMedia
        );

private:
    static HRESULT ChoosePlayer(
        _In_        MediaInstance       *pMediaInstance,
        _In_        bool                canOpenAnyMedia,
        __deref_out IMILMedia           **ppMedia
        );
};

class NoDllRefCount
{
public:

    static inline
    void
    AddRef(
        void
        );

    static inline
    void
    Release(
        void
        );
};

template<class Base, class DllCount>
class RealComObject : public Base
{
public:

    RealComObject(
        void
        );

    template<typename P1>
    RealComObject(
        _In_    P1      p1
        );

    template<typename P1, typename P2>
    RealComObject(
        _In_    P1      p1,
        _In_    P2      p2
        );


    template<typename P1, typename P2, typename P3>
    RealComObject(
        _In_    P1      p1,
        _In_    P2      p2,
        _In_    P3      p3
        );

    template<typename P1, typename P2, typename P3, typename P4>
    RealComObject(
        _In_    P1      p1,
        _In_    P2      p2,
        _In_    P3      p3,
        _In_    P4      p4
        );

    template<typename P1, typename P2, typename P3, typename P4, typename P5>
    RealComObject(
        _In_    P1      p1,
        _In_    P2      p2,
        _In_    P3      p3,
        _In_    P4      p4,
        _In_    P5      p5
        );

    template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
    RealComObject(
        _In_    P1      p1,
        _In_    P2      p2,
        _In_    P3      p3,
        _In_    P4      p4,
        _In_    P5      p5,
        _In_    P6      p6
        );

    ~RealComObject(
        void
        );

    STDMETHOD(QueryInterface)(
        _In_        REFIID      riid,
        __deref_out void        **ppv
        );

    STDMETHOD_(ULONG, AddRef)(
        void
        );

    STDMETHOD_(ULONG, Release)(
        void
        );

private:

    //
    // Cannot copy or assign a RealComObject
    //
    RealComObject(
        _In_    const RealComObject &
        );

    RealComObject &
    operator=(
        _In_    const RealComObject &
        );

    inline
    void
    Construct(
        void
        );

    LONG        m_cRef;
};

#include "../av/milav.inl"


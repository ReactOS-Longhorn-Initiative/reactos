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
//  $ENDTAG
//
//------------------------------------------------------------------------------
/*static*/ inline
void
NoDllRefCount::
AddRef(
    void
    )
{
}

/*static*/ inline
void
NoDllRefCount::
Release(
    void
    )
{
}

//
// RealComObject implementation
//
template<class Base, class DllRefCount>
RealComObject<Base, DllRefCount>::
RealComObject(
    void
    ) : Base()
{
    Construct();
}

template<class Base, class DllRefCount>
template<typename P1>
RealComObject<Base, DllRefCount>::
RealComObject(
    _In_    P1      p1
    ) : Base(p1)
{
    Construct();
}

template<class Base, class DllRefCount>
template<typename P1, typename P2>
RealComObject<Base, DllRefCount>::
RealComObject(
    _In_    P1      p1,
    _In_    P2      p2
    ) : Base(p1, p2)
{
    Construct();
}

template<class Base, class DllRefCount>
template<typename P1, typename P2, typename P3>
RealComObject<Base, DllRefCount>::
RealComObject(
    _In_    P1      p1,
    _In_    P2      p2,
    _In_    P3      p3
    ) : Base(p1, p2, p3)
{
    Construct();
}

template<class Base, class DllRefCount>
template<typename P1, typename P2, typename P3, typename P4>
RealComObject<Base, DllRefCount>::
RealComObject(
    _In_    P1      p1,
    _In_    P2      p2,
    _In_    P3      p3,
    _In_    P4      p4
    ) : Base(p1, p2, p3, p4)
{
    Construct();
}

template<class Base, class DllRefCount>
template<typename P1, typename P2, typename P3, typename P4, typename P5>
RealComObject<Base, DllRefCount>::
RealComObject(
    _In_    P1      p1,
    _In_    P2      p2,
    _In_    P3      p3,
    _In_    P4      p4,
    _In_    P5      p5
    ) : Base(p1, p2, p3, p4, p5)
{
    Construct();
}

template<class Base, class DllRefCount>
template<typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
RealComObject<Base, DllRefCount>::
RealComObject(
    _In_    P1      p1,
    _In_    P2      p2,
    _In_    P3      p3,
    _In_    P4      p4,
    _In_    P5      p5,
    _In_    P6      p6
    ) : Base(p1, p2, p3, p4, p5, p6)
{
    Construct();
}

template<class Base, class DllRefCount>
RealComObject<Base, DllRefCount>::
~RealComObject(
    void
    )
{
    DllRefCount::Release();
}

template<class Base, class DllRefCount>
STDMETHODIMP
RealComObject<Base, DllRefCount>::
QueryInterface(
    _In_        REFIID      riid,
    __deref_out void        **ppv
    )
{
    //
    // This would be nicer as a typelist implementation, but, that
    // is pretty complicated and this is simple.
    //
    HRESULT     hr = S_OK;
    void *pv;

    if (!ppv)
    {
        IFCN(E_INVALIDARG);
    }

    pv = Base::GetInterface(riid);

    if (NULL == pv)
    {
        IFCN(E_NOINTERFACE);
    }

    *ppv = pv;

    RealComObject::AddRef();

Cleanup:

    RRETURN(hr);
}
template<class Base, class DllRefCount>
STDMETHODIMP_(ULONG)
RealComObject<Base, DllRefCount>::
AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}

template<class Base, class DllRefCount>
STDMETHODIMP_(ULONG)
RealComObject<Base, DllRefCount>::
Release(
    void
    )
{
    LONG    cRef = InterlockedDecrement(&m_cRef);

    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

template<class Base, class DllRefCount>
inline
void
RealComObject<Base, DllRefCount>::
Construct(
    void
    )
{
    m_cRef = 1;

    DllRefCount::AddRef();
}



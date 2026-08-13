// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  File:       Locks.h
//------------------------------------------------------------------------------
#pragma once

//+----------------------------------------------------------------------------
//
//  Class:      CCriticalSection
//
//  Synopsis:   Represents a 'critical section' - a synchronization object
//              that allows one thread at a time to access a resource or
//              section of code.
//
//  Usage:      To use a CCriticalSection object, construct the CCriticalSection
//              object when it is needed and then call Init().
//              NOTE: Init() and Enter() can fail.
//
//  Notes:      This uses two-stage initialization to protect against failure 
//              on downlevel operating system platforms.
//
//-----------------------------------------------------------------------------

class CCriticalSection
{
public:
    CCriticalSection();
    ~CCriticalSection();

    HRESULT Init();
    void DeInit();

    void Enter();
    bool TryEnter();
    void Leave();

    HANDLE OwningThread() const;

    bool IsValid() const;

private:
    CRITICAL_SECTION _cs;
    bool             _fInited;
};

inline CCriticalSection::CCriticalSection()
{
    _fInited = false;
}

inline CCriticalSection::~CCriticalSection()
{
    DeInit();
}

inline HRESULT CCriticalSection::Init()
{
    Assert(!_fInited);

    HRESULT hr = S_OK;

    //
    // IFCW32 contains a comparison that may be constant depending on
    // compilation options. Skip the compiler warning for that.
    //

    IFCW32(InitializeCriticalSectionAndSpinCount(&_cs, 0));

    _fInited = true;

Cleanup:
    RRETURN(hr);
}

inline void CCriticalSection::DeInit()
{
    if (_fInited)
    {
        DeleteCriticalSection(&_cs);
        _fInited = false;
    }
}

inline void CCriticalSection::Enter()
{
    Assert(_fInited);
    EnterCriticalSection(&_cs);
}

inline bool CCriticalSection::TryEnter()
{
    Assert(_fInited);
    return (TryEnterCriticalSection(&_cs) != 0);
}

inline void CCriticalSection::Leave()
{
    Assert(_fInited);
    LeaveCriticalSection(&_cs);
}

inline HANDLE CCriticalSection::OwningThread() const
{
    Assert(_fInited);
    return _cs.OwningThread;
}

inline bool CCriticalSection::IsValid() const
{
    return _fInited;
}

//+----------------------------------------------------------------------------
//
//  Class:      CGuard
//
//  Synopsis:   Simplifies usage of synchronization objects by automatic
//              lock/unlock.
//
//-----------------------------------------------------------------------------

//
// THIS WAS A NO-OP. Every method was empty and _pLock was never assigned, so
// every `CGuard<CCriticalSection> oGuard(x)` in milcore locked nothing at all.
//
// It surfaced as CMilMasterHandleTable::GetEntry's assertion
//
//     Unsynchronized access to the handle-table
//     HandleToULong(g_csCompositionEngine.OwningThread()) == GetCurrentThreadId()
//
// firing from CreateOrAddRefOnChannel, which takes the guard on the line
// before. The assert was right and the guard was lying: nothing held the lock,
// so OwningThread() was whatever last happened to enter the CS.
//
// The comment in CMilChannel::Commit -- "FlushChannelHandlers releases the
// composition CS when its guard ends" -- described behaviour that did not
// exist either.
//
// CCriticalSection wraps a CRITICAL_SECTION, which is recursive on the same
// thread, so nested guards are safe. All seven locks used with CGuard are
// Init()ed before first use (checked) -- worth re-checking before adding an
// eighth, because an uninitialised lock could not fault while this was inert.
//
template<typename LOCK> class CGuard
{
public:
    CGuard(__inout_ecount(1) LOCK &lock)
        : _pLock(&lock)
    {
        _pLock->Enter();
    }

    ~CGuard()
    {
        if (_pLock)
        {
            _pLock->Leave();
            _pLock = NULL;
        }
    }

    //
    // Release early. The destructor must not release a second time, which is
    // why this clears the pointer rather than just calling Leave.
    //
    void Leave()
    {
        if (_pLock)
        {
            _pLock->Leave();
            _pLock = NULL;
        }
    }

private:
    LOCK * _pLock;

    // A copied guard would release twice.
    CGuard(const CGuard &);
    CGuard &operator=(const CGuard &);
};

template<typename Lock> class CUnGuard
{
public:

    CUnGuard(
        __inout_ecount(1) Lock &lock
        )
    { 
 
    }

    ~CUnGuard()                         
    { 
    
    }

private:

    Lock        *m_pLock;
};


#pragma once

// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include <unknwn.h>

#include "../../common/shared/milcom.h"

//
// IMilRedirectedGDISurface represents a redirected GDI surface exposed by DWM.
// The interface is intentionally minimal for now; methods return E_NOTIMPL until
// the backing implementation is completed.
//

class CMilServerConnectionManager;

EXTERN_C const IID IID_IMilRedirectedGDISurface;

struct IMilRedirectedGDISurface : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetInformation(
        _In_ DWORD informationClass,
        _Out_writes_bytes_opt_(*pcbSize) UINT *pcbSize,
        _Out_writes_bytes_opt_(*pcbSize) void *pData) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetInformation(
        _In_ DWORD informationClass,
        _In_ UINT cbData,
        _In_reads_bytes_opt_(cbData) const void *pData) = 0;
};

EXTERN_C const IID IID_IMilRedirectedGDISurfaceManager;

struct IMilRedirectedGDISurfaceManager : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE RegisterServerConnectionManager(
        _In_opt_ CMilServerConnectionManager *pServerConnectionManager) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetPresentHistory(
        _In_ UINT cbHistory,
        _Out_writes_bytes_opt_(cbHistory) BYTE *pbHistory,
        _Out_opt_ UINT *pcbWritten) = 0;

    virtual HRESULT STDMETHODCALLTYPE ResetPresentHistory() = 0;

    virtual HRESULT STDMETHODCALLTYPE UsePresentHistory(
        _In_ BOOL fEnable) = 0;

    virtual HRESULT STDMETHODCALLTYPE UsingPresentHistory(
        _Out_ BOOL *pfEnabled) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetRedirSurface(
        _In_ ULONGLONG hSprite,
        _Outptr_result_maybenull_ IMilRedirectedGDISurface **ppSurface) = 0;
};

class CMilRedirectedGDISurface;
class CMilConnectionManager;
class CMilServerConnectionManager;

class CMilSurfaceManager final :
    public CMILCOMBase,
    public IMilRedirectedGDISurfaceManager
{
public:
    DECLARE_COM_BASE;

    static HRESULT Create(_Outptr_ IMilRedirectedGDISurfaceManager **ppManager);

    // IUnknown support via CMILCOMBase.
    HRESULT STDMETHODCALLTYPE HrFindInterface(REFIID riid, void **ppvObject) override;

    // IMilRedirectedGDISurfaceManager
    HRESULT STDMETHODCALLTYPE RegisterServerConnectionManager(
        _In_opt_ CMilServerConnectionManager *pServerConnectionManager) override;
    HRESULT STDMETHODCALLTYPE GetPresentHistory(UINT cbHistory, BYTE *pbHistory, UINT *pcbWritten) override;
    HRESULT STDMETHODCALLTYPE ResetPresentHistory() override;
    HRESULT STDMETHODCALLTYPE UsePresentHistory(BOOL fEnable) override;
    HRESULT STDMETHODCALLTYPE UsingPresentHistory(BOOL *pfEnabled) override;
    HRESULT STDMETHODCALLTYPE GetRedirSurface(ULONGLONG hSprite, IMilRedirectedGDISurface **ppSurface) override;

private:
    CMilSurfaceManager();
    ~CMilSurfaceManager() override;

    BOOL m_fUsePresentHistory;
    CMilServerConnectionManager *m_pServerConnectionManager;
};

class CMilRedirectedGDISurface final :
    public CMILCOMBase,
    public IMilRedirectedGDISurface
{
public:
    DECLARE_COM_BASE;

    static HRESULT Create(
        _In_ CMilSurfaceManager *pOwner,
        _In_ ULONGLONG hSprite,
        _Outptr_ IMilRedirectedGDISurface **ppSurface);

    // IUnknown support via CMILCOMBase.
    HRESULT STDMETHODCALLTYPE HrFindInterface(REFIID riid, void **ppvObject) override;

    // IMilRedirectedGDISurface
    HRESULT STDMETHODCALLTYPE GetInformation(DWORD informationClass, UINT *pcbSize, void *pData) override;
    HRESULT STDMETHODCALLTYPE SetInformation(DWORD informationClass, UINT cbData, const void *pData) override;

private:
    CMilRedirectedGDISurface(CMilSurfaceManager *pOwner, ULONGLONG hSprite);
    ~CMilRedirectedGDISurface() override;

    CMilSurfaceManager *m_pOwner; // weak ref
    ULONGLONG m_hSprite;
};

class CMilConnectionManager final : public CMILCOMBase
{
public:
    DECLARE_COM_BASE;

    static HRESULT Create(
        _In_opt_ IMilRedirectedGDISurfaceManager *pSurfaceManager,
        _Outptr_ CMilConnectionManager **ppManager);

    HRESULT STDMETHODCALLTYPE HrFindInterface(REFIID riid, void **ppvObject) override;

    IMilRedirectedGDISurfaceManager *GetSurfaceManagerNoRef() const { return m_pSurfaceManager; }

private:
    HRESULT InitializeTransportManager();

    explicit CMilConnectionManager(IMilRedirectedGDISurfaceManager *pSurfaceManager);
    ~CMilConnectionManager() override;

    IMilRedirectedGDISurfaceManager *m_pSurfaceManager;
    CMilServerConnectionManager *m_pServerConnectionManager;
};

class CMilServerConnectionManager final : public CMILCOMBase
{
public:
    DECLARE_COM_BASE;

    static HRESULT Create(
        _In_ CMilConnectionManager *pOwner,
        _In_opt_ IMilRedirectedGDISurfaceManager *pSurfaceManager,
        _Outptr_ CMilServerConnectionManager **ppManager);

    HRESULT STDMETHODCALLTYPE HrFindInterface(REFIID riid, void **ppvObject) override;

    IMilRedirectedGDISurfaceManager *GetSurfaceManagerNoRef() const { return m_pSurfaceManager; }

private:
    explicit CMilServerConnectionManager(_In_ CMilConnectionManager *pOwner);
    ~CMilServerConnectionManager() override;

    HRESULT Initialize(_In_opt_ IMilRedirectedGDISurfaceManager *pSurfaceManager);

    CMilConnectionManager *m_pOwner; // weak ref
    IMilRedirectedGDISurfaceManager *m_pSurfaceManager;
};

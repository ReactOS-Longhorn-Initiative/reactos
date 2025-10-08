// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

#include "precomp.hpp"
#include "transport.h"

#include <new>
#include <debug.h>

ExternTag(tagMILTransport);

DEFINE_GUID(IID_IMilRedirectedGDISurface,
            0xb43973e6, 0x694f, 0x48d8, 0xa4, 0x40, 0x44, 0xa4, 0x7e, 0x67, 0x4e, 0xc3);

DEFINE_GUID(IID_IMilRedirectedGDISurfaceManager,
            0xd2afa5f8, 0x2a1b, 0x4a0e, 0x80, 0xfd, 0xdc, 0x18, 0x57, 0xa3, 0xae, 0x65);

// CMilSurfaceManager -----------------------------------------------------------------

CMilSurfaceManager::CMilSurfaceManager()
    : m_fUsePresentHistory(FALSE)
    , m_pServerConnectionManager(nullptr)
{
}

CMilSurfaceManager::~CMilSurfaceManager()
{
    if (m_pServerConnectionManager)
    {
        (void)RegisterServerConnectionManager(nullptr);
    }

    TraceTag((tagMILTransport, "CMilSurfaceManager::~CMilSurfaceManager: destroyed manager=%p", this));
}

/*static*/ HRESULT
CMilSurfaceManager::Create(_Outptr_ IMilRedirectedGDISurfaceManager **ppManager)
{
    if (!ppManager)
    {
        TraceTag((tagMILTransport, "CMilSurfaceManager::Create: invalid manager pointer"));
        return E_INVALIDARG;
    }

    *ppManager = nullptr;

    CMilSurfaceManager *pManager = new (std::nothrow) CMilSurfaceManager();
    if (!pManager)
    {
        TraceTag((tagMILTransport, "CMilSurfaceManager::Create: out of memory"));
        return E_OUTOFMEMORY;
    }

    pManager->AddRef();
    *ppManager = pManager;
    TraceTag((tagMILTransport, "CMilSurfaceManager::Create: success manager=%p", pManager));
    return S_OK;
}

HRESULT WINAPI
CMilSurfaceManager::HrFindInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
    {
        return E_INVALIDARG;
    }

    if (riid == IID_IMilRedirectedGDISurfaceManager)
    {
        *ppvObject = static_cast<IMilRedirectedGDISurfaceManager *>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE
CMilSurfaceManager::RegisterServerConnectionManager(
    _In_opt_ CMilServerConnectionManager *pServerConnectionManager)
{
    TraceTag((tagMILTransport,
              "CMilSurfaceManager::RegisterServerConnectionManager: manager=%p server=%p",
              this,
              pServerConnectionManager));

    if (m_pServerConnectionManager == pServerConnectionManager)
    {
        return S_OK;
    }

    if (m_pServerConnectionManager)
    {
        m_pServerConnectionManager->Release();
        m_pServerConnectionManager = nullptr;
    }

    if (pServerConnectionManager)
    {
        pServerConnectionManager->AddRef();
    }

    m_pServerConnectionManager = pServerConnectionManager;
    return S_OK;
}

HRESULT
WINAPI
CMilSurfaceManager::GetPresentHistory(UINT cbHistory, BYTE *pbHistory, UINT *pcbWritten)
{
    __debugbreak();
    UNREFERENCED_PARAMETER(cbHistory);
    if (pbHistory && cbHistory > 0)
    {
        RtlZeroMemory(pbHistory, cbHistory);
    }
    if (pcbWritten)
    {
        *pcbWritten = 0;
    }

    TraceTag((tagMILTransport, "CMilSurfaceManager::GetPresentHistory: stubbed (present history unsupported)"));
    return E_NOTIMPL;
}

// CMilServerConnectionManager --------------------------------------------------------

CMilServerConnectionManager::CMilServerConnectionManager(_In_ CMilConnectionManager *pOwner)
    : m_pOwner(pOwner)
    , m_pSurfaceManager(nullptr)
{
    TraceTag((tagMILTransport,
              "CMilServerConnectionManager::CMilServerConnectionManager: created manager=%p owner=%p",
              this,
              m_pOwner));
}

CMilServerConnectionManager::~CMilServerConnectionManager()
{
    if (m_pSurfaceManager)
    {
        (void)m_pSurfaceManager->RegisterServerConnectionManager(nullptr);
        m_pSurfaceManager->Release();
        m_pSurfaceManager = nullptr;
    }

    TraceTag((tagMILTransport,
              "CMilServerConnectionManager::~CMilServerConnectionManager: destroyed manager=%p",
              this));
}

/*static*/ HRESULT
CMilServerConnectionManager::Create(
    _In_ CMilConnectionManager *pOwner,
    _In_opt_ IMilRedirectedGDISurfaceManager *pSurfaceManager,
    _Outptr_ CMilServerConnectionManager **ppManager)
{
    if (!ppManager)
    {
        TraceTag((tagMILTransport, "CMilServerConnectionManager::Create: invalid manager pointer"));
        return E_INVALIDARG;
    }

    *ppManager = nullptr;

    CMilServerConnectionManager *pManager = new (std::nothrow) CMilServerConnectionManager(pOwner);
    if (!pManager)
    {
        TraceTag((tagMILTransport,
                  "CMilServerConnectionManager::Create: out of memory owner=%p surfaceManager=%p",
                  pOwner,
                  pSurfaceManager));
        return E_OUTOFMEMORY;
    }

    pManager->AddRef();

    HRESULT hr = pManager->Initialize(pSurfaceManager);
    if (FAILED(hr))
    {
        TraceTag((tagMILTransport,
                  "CMilServerConnectionManager::Create: initialization failed hr=0x%08x",
                  hr));
        pManager->Release();
        return hr;
    }

    *ppManager = pManager;
    TraceTag((tagMILTransport,
              "CMilServerConnectionManager::Create: success manager=%p surfaceManager=%p",
              pManager,
              pSurfaceManager));
    return S_OK;
}

HRESULT
CMilServerConnectionManager::Initialize(_In_opt_ IMilRedirectedGDISurfaceManager *pSurfaceManager)
{
    if (m_pSurfaceManager == pSurfaceManager)
    {
        TraceTag((tagMILTransport,
                  "CMilServerConnectionManager::Initialize: unchanged manager=%p surfaceManager=%p",
                  this,
                  m_pSurfaceManager));
        return S_OK;
    }

    if (m_pSurfaceManager)
    {
        (void)m_pSurfaceManager->RegisterServerConnectionManager(nullptr);
        m_pSurfaceManager->Release();
        m_pSurfaceManager = nullptr;
    }

    if (pSurfaceManager)
    {
        pSurfaceManager->AddRef();
    }

    m_pSurfaceManager = pSurfaceManager;

    if (m_pSurfaceManager)
    {
        HRESULT hrRegister = m_pSurfaceManager->RegisterServerConnectionManager(this);
        if (FAILED(hrRegister))
        {
            TraceTag((tagMILTransport,
                      "CMilServerConnectionManager::Initialize: register failed hr=0x%08x",
                      hrRegister));
            m_pSurfaceManager->Release();
            m_pSurfaceManager = nullptr;
            return hrRegister;
        }
    }

    TraceTag((tagMILTransport,
              "CMilServerConnectionManager::Initialize: manager=%p surfaceManager=%p",
              this,
              m_pSurfaceManager));
    return S_OK;
}

HRESULT
CMilServerConnectionManager::HrFindInterface(REFIID riid, void **ppvObject)
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(ppvObject);
    TraceTag((tagMILTransport, "CMilServerConnectionManager::HrFindInterface: unsupported interface"));
    return E_NOINTERFACE;
}

HRESULT WINAPI
CMilSurfaceManager::ResetPresentHistory()
{
    __debugbreak();
    m_fUsePresentHistory = FALSE;
    TraceTag((tagMILTransport, "CMilSurfaceManager::ResetPresentHistory: disabled present history"));
    return S_OK;
}

HRESULT WINAPI
CMilSurfaceManager::UsePresentHistory(BOOL fEnable)
{
    __debugbreak();
    m_fUsePresentHistory = fEnable ? TRUE : FALSE;
    TraceTag((tagMILTransport, "CMilSurfaceManager::UsePresentHistory: set=%d", m_fUsePresentHistory));
    return S_OK;
}

HRESULT WINAPI
CMilSurfaceManager::UsingPresentHistory(BOOL *pfEnabled)
{
    __debugbreak();
    if (!pfEnabled)
    {
        TraceTag((tagMILTransport, "CMilSurfaceManager::UsingPresentHistory: invalid enabled pointer"));
        return E_INVALIDARG;
    }

    *pfEnabled = m_fUsePresentHistory;
    TraceTag((tagMILTransport, "CMilSurfaceManager::UsingPresentHistory: returning=%d", *pfEnabled));
    return S_OK;
}

HRESULT WINAPI
CMilSurfaceManager::GetRedirSurface(ULONGLONG hSprite, IMilRedirectedGDISurface **ppSurface)
{
    __debugbreak();
    if (!ppSurface)
    {
        TraceTag((tagMILTransport, "CMilSurfaceManager::GetRedirSurface: invalid surface pointer"));
        return E_INVALIDARG;
    }

    *ppSurface = nullptr;

    HRESULT hr = CMilRedirectedGDISurface::Create(this, hSprite, ppSurface);
    if (SUCCEEDED(hr))
    {
        TraceTag((tagMILTransport,
                  "CMilSurfaceManager::GetRedirSurface: created surface=%p for sprite=%I64u",
                  *ppSurface,
                  hSprite));
    }
    else
    {
        TraceTag((tagMILTransport,
                  "CMilSurfaceManager::GetRedirSurface: failed sprite=%I64u hr=0x%08x",
                  hSprite,
                  hr));
    }

    return hr;
}

// CMilRedirectedGDISurface -------------------------------------------------------------

CMilRedirectedGDISurface::CMilRedirectedGDISurface(CMilSurfaceManager *pOwner, ULONGLONG hSprite)
    : m_pOwner(pOwner)
    , m_hSprite(hSprite)
{
    if (m_pOwner)
    {
        m_pOwner->AddRef();
    }
}

CMilRedirectedGDISurface::~CMilRedirectedGDISurface()
{
    if (m_pOwner)
    {
        m_pOwner->Release();
        m_pOwner = nullptr;
    }

    TraceTag((tagMILTransport,
              "CMilRedirectedGDISurface::~CMilRedirectedGDISurface: destroyed surface sprite=%I64u",
              m_hSprite));
}

/*static*/ HRESULT
CMilRedirectedGDISurface::Create(
    _In_ CMilSurfaceManager *pOwner,
    _In_ ULONGLONG hSprite,
    _Outptr_ IMilRedirectedGDISurface **ppSurface)
{
    if (!ppSurface)
    {
        TraceTag((tagMILTransport, "CMilRedirectedGDISurface::Create: invalid surface pointer"));
        return E_INVALIDARG;
    }

    *ppSurface = nullptr;

    CMilRedirectedGDISurface *pSurface = new (std::nothrow) CMilRedirectedGDISurface(pOwner, hSprite);
    if (!pSurface)
    {
        TraceTag((tagMILTransport,
                  "CMilRedirectedGDISurface::Create: out of memory sprite=%I64u",
                  hSprite));
        return E_OUTOFMEMORY;
    }

    pSurface->AddRef();
    *ppSurface = pSurface;
    TraceTag((tagMILTransport,
              "CMilRedirectedGDISurface::Create: success surface=%p sprite=%I64u",
              pSurface,
              hSprite));
    return S_OK;
}

STDMETHODIMP
CMilRedirectedGDISurface::HrFindInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
    {
        return E_INVALIDARG;
    }

    if (riid == IID_IMilRedirectedGDISurface)
    {
        *ppvObject = static_cast<IMilRedirectedGDISurface *>(this);
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT WINAPI
CMilRedirectedGDISurface::GetInformation(DWORD /*informationClass*/, UINT * /*pcbSize*/, void * /*pData*/)
{
    TraceTag((tagMILTransport,
              "CMilRedirectedGDISurface::GetInformation: stubbed sprite=%I64u",
              m_hSprite));
    return E_NOTIMPL;
}

HRESULT WINAPI
CMilRedirectedGDISurface::SetInformation(DWORD /*informationClass*/, UINT /*cbData*/, const void * /*pData*/)
{
    TraceTag((tagMILTransport,
              "CMilRedirectedGDISurface::SetInformation: stubbed sprite=%I64u",
              m_hSprite));
    return E_NOTIMPL;
}

// CMilConnectionManager ---------------------------------------------------------------

CMilConnectionManager::CMilConnectionManager(IMilRedirectedGDISurfaceManager *pSurfaceManager)
    : m_pSurfaceManager(pSurfaceManager)
    , m_pServerConnectionManager(nullptr)
{
    if (m_pSurfaceManager)
    {
        m_pSurfaceManager->AddRef();
    }

    TraceTag((tagMILTransport,
              "CMilConnectionManager::CMilConnectionManager: created manager=%p surfaceManager=%p",
              this,
              m_pSurfaceManager));
}

CMilConnectionManager::~CMilConnectionManager()
{
    if (m_pServerConnectionManager)
    {
        m_pServerConnectionManager->Release();
        m_pServerConnectionManager = nullptr;
    }

    if (m_pSurfaceManager)
    {
        m_pSurfaceManager->Release();
        m_pSurfaceManager = nullptr;
    }

    TraceTag((tagMILTransport,
              "CMilConnectionManager::~CMilConnectionManager: destroyed manager=%p",
              this));
}

/*static*/ HRESULT
CMilConnectionManager::Create(
    _In_opt_ IMilRedirectedGDISurfaceManager *pSurfaceManager,
    _Outptr_ CMilConnectionManager **ppManager)
{
    if (!ppManager)
    {
        return E_INVALIDARG;
    }

    *ppManager = nullptr;

    CMilConnectionManager *pManager = new (std::nothrow) CMilConnectionManager(pSurfaceManager);
    if (!pManager)
    {
        TraceTag((tagMILTransport, "CMilConnectionManager::Create: out of memory"));
        return E_OUTOFMEMORY;
    }

    pManager->AddRef();

    HRESULT hr = pManager->InitializeTransportManager();
    if (FAILED(hr))
    {
        TraceTag((tagMILTransport,
                  "CMilConnectionManager::Create: initialize transport manager failed hr=0x%08x",
                  hr));
        pManager->Release();
        return hr;
    }

    *ppManager = pManager;
    TraceTag((tagMILTransport,
              "CMilConnectionManager::Create: success manager=%p surfaceManager=%p",
              pManager,
              pSurfaceManager));
    return S_OK;
}

HRESULT
CMilConnectionManager::HrFindInterface(REFIID riid, void **ppvObject)
{
    UNREFERENCED_PARAMETER(riid);
    UNREFERENCED_PARAMETER(ppvObject);
    TraceTag((tagMILTransport, "CMilConnectionManager::HrFindInterface: unsupported interface"));
    return E_NOINTERFACE;
}

HRESULT
CMilConnectionManager::InitializeTransportManager()
{
    TraceTag((tagMILTransport,
              "CMilConnectionManager::InitializeTransportManager: manager=%p surfaceManager=%p",
              this,
              m_pSurfaceManager));

    if (m_pServerConnectionManager)
    {
        TraceTag((tagMILTransport,
                  "CMilConnectionManager::InitializeTransportManager: already initialized manager=%p",
                  m_pServerConnectionManager));
        return S_OK;
    }

    CMilServerConnectionManager *pServerManager = nullptr;
    HRESULT hr = CMilServerConnectionManager::Create(this, m_pSurfaceManager, &pServerManager);
    if (FAILED(hr))
    {
        TraceTag((tagMILTransport,
                  "CMilConnectionManager::InitializeTransportManager: failed hr=0x%08x",
                  hr));
        return hr;
    }

    m_pServerConnectionManager = pServerManager;
    TraceTag((tagMILTransport,
              "CMilConnectionManager::InitializeTransportManager: initialized serverManager=%p",
              m_pServerConnectionManager));
    return S_OK;
}

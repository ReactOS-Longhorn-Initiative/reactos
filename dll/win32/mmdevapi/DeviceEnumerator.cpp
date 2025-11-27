/*
 * PROJECT:         ReactOS WAS Audio Stack implementation
 * LICENSE:         MIT
 * FILE:            dll/win32/mmdevapi/DeviceEnumerator.cpp
 * PURPOSE:         Implemenation of MMDeviceEnumerator
 * PROGRAMMERS:     Mikhail Tyukin (mishakey20@gmail.com)
 */
#define COBJMACROS

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

class CDeviceEnumerator : public IMMDeviceEnumerator
{
public:
    CDeviceEnumerator();
    virtual ~CDeviceEnumerator();

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IMMDeviceEnumerator methods **
    STDMETHODIMP EnumAudioEndpoints(_In_ EDataFlow dataFlow, _In_ DWORD dwStateMask, _Out_ IMMDeviceCollection** ppDevices);
    STDMETHODIMP GetDefaultAudioEndpoint(_In_ EDataFlow dataFlow, _In_ ERole ole, _Out_ IMMDevice** ppEndpoint);
    STDMETHODIMP GetDevice(_In_ LPCWSTR id, _Out_ IMMDevice **ppDevice);
    STDMETHODIMP RegisterEndpointNotificationCallback(_In_ IMMNotificationClient* client);
    STDMETHODIMP UnregisterEndpointNotificationCallback(_In_ IMMNotificationClient* client);

    // Helpers
    static HRESULT CreateInstance(IUnknown *pUnkOuter, CDeviceEnumerator **ppOut)
    {
        HRESULT hr = E_OUTOFMEMORY;

        CDeviceEnumerator* instance = new CDeviceEnumerator();
        if (instance)
        {
            *ppOut = instance;
            return S_OK;
        }

        return hr;
    }

private:
    LONG m_cRefs;
};

CDeviceEnumerator::CDeviceEnumerator()
{
    m_cRefs = 1;
}

CDeviceEnumerator::~CDeviceEnumerator()
{

}

STDMETHODIMP_(ULONG) CDeviceEnumerator::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CDeviceEnumerator::Release()
{
    ULONG ret = InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CDeviceEnumerator::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;
    if (riid == IID_IMMDeviceEnumerator || riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = static_cast<IMMDeviceEnumerator*>(this);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

HRESULT CDeviceEnumerator::EnumAudioEndpoints(_In_ EDataFlow dataFlow, _In_ DWORD dwStateMask, _Out_ IMMDeviceCollection** ppDevices)
{
    FIXME("Not implemented: CDeviceEnumerator::EnumAudioEndpoints\n");
    return E_NOTIMPL;
}

HRESULT CDeviceEnumerator::GetDefaultAudioEndpoint(_In_ EDataFlow dataFlow, _In_ ERole ole, _Out_ IMMDevice** ppEndpoint)
{
    FIXME("Not implemented: CDeviceEnumerator::GetDefaultAudioEndpoint\n");
    return E_NOTIMPL;
}

HRESULT CDeviceEnumerator::GetDevice(_In_ LPCWSTR id, _Out_ IMMDevice **ppDevice)
{
    FIXME("Not implemented: CDeviceEnumerator::GetDevice\n");
    return E_NOTIMPL;
}

HRESULT CDeviceEnumerator::RegisterEndpointNotificationCallback(_In_ IMMNotificationClient* client)
{
    FIXME("Not implemented: CDeviceEnumerator::RegisterEndpointNotificationCallback\n");
    return E_NOTIMPL;
}

HRESULT CDeviceEnumerator::UnregisterEndpointNotificationCallback(_In_ IMMNotificationClient* client)
{
    FIXME("Not implemented: CDeviceEnumerator::UnregisterEndpointNotificationCallback\n");
    return E_NOTIMPL;
}

EXTERN_C HRESULT MMDeviceEnumerator_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    return CDeviceEnumerator::CreateInstance(pUnkOuter, (CDeviceEnumerator**)ppOut);
}

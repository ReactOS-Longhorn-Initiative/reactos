/*
 * PROJECT:         ReactOS WAS Audio Stack implementation
 * LICENSE:         MIT
 * FILE:            dll/win32/mmdevapi/factory.cpp
 * PURPOSE:         COM Factory
 * PROGRAMMERS:     Mikhail Tyukin (mishakey20@gmail.com)
 */
#define COBJMACROS

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

static HINSTANCE g_hInstance = NULL;

typedef HRESULT (*LPFNCONSTRUCTOR)(IUnknown *pUnkOuter, IUnknown **ppvOut);

static const struct {
    const CLSID *clsid;
    LPFNCONSTRUCTOR ctor;
} ClassesTable[] = {
    {&CLSID_MMDeviceEnumerator, MMDeviceEnumerator_Constructor},
    {NULL, NULL}
};

class CClassFactory
    : public IClassFactory
{
public:
    CClassFactory(LPFNCONSTRUCTOR ctor);
    virtual ~CClassFactory();

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** IClassFactory methods **
    STDMETHODIMP CreateInstance(
        _In_ IUnknown *pUnkOuter,
        _In_ REFIID riid,
        _Out_ void **ppvObject) override;
    STDMETHODIMP LockServer(_In_ BOOL fLock) override;

protected:
    LONG m_cRefs;
    LPFNCONSTRUCTOR m_ctor;
};

CClassFactory::CClassFactory(LPFNCONSTRUCTOR ctor)
    : m_cRefs(1)
    , m_ctor(ctor)
{
}

CClassFactory::~CClassFactory()
{
    TRACE("Destroying class factory %p\n", this);
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;
    if (riid == IID_IClassFactory || riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = static_cast<IClassFactory *>(this);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    ULONG ret = InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CClassFactory::CreateInstance(
    _In_ IUnknown *pUnkOuter,
    _In_ REFIID riid,
    _Out_ void **ppvObject)
{
    TRACE("(%p, %p, %s, %p)\n", this, pUnkOuter, debugstr_guid(&riid), ppvObject);

    IUnknown *obj;
    HRESULT ret = m_ctor(pUnkOuter, &obj);
    if (FAILED(ret))
        return ret;
    ret = obj->QueryInterface(riid, ppvObject);
    obj->Release();
    return ret;
}

STDMETHODIMP CClassFactory::LockServer(_In_ BOOL fLock)
{
    TRACE("(%p)->(%x)\n", this, fLock);
    return S_OK;
}

static HRESULT ClassFactory_Constructor(LPFNCONSTRUCTOR ctor, LPVOID *ppvOut)
{
    CClassFactory *This = new CClassFactory(ctor);
    *ppvOut = static_cast<IClassFactory *>(This);
    TRACE("Created class factory %p\n", This);
    return S_OK;
}

EXTERN_C HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppvOut)
{
    int i;

    *ppvOut = NULL;
    if (iid != IID_IUnknown && iid != IID_IClassFactory)
        return E_NOINTERFACE;

    for (i = 0; ClassesTable[i].clsid; i++)
    {
        if (*ClassesTable[i].clsid == clsid)
            return ClassFactory_Constructor(ClassesTable[i].ctor, ppvOut);
    }
    FIXME("CLSID %s not supported\n", debugstr_guid(&clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}


EXTERN_C BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

EXTERN_C HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources(g_hInstance);
}

EXTERN_C HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources(g_hInstance);
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

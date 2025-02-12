/*
 * shpolicy.c - Data for shell/system policies.
 *
 * Copyright 1999 Ian Schmidt <ischmidt@cfl.rr.com>
 * Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES:
 *
 * Some of these policies can be tweaked via the System Policy
 * Editor which came with the Win95 Migration Guide, although
 * there doesn't appear to be an updated Win98 version that
 * would handle the many new policies introduced since then.
 * You could easily write one with the information in
 * this file...
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <initguid.h>
#include <propsys.h>
#include <shlwapi_undoc.h>
#include <wine/debug.h>

#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

DEFINE_GUID(GUID_Restrictions, 0xA48F1A32, 0xA340, 0x11D1, 0xBC, 0x6B, 0x00, 0xA0, 0xC9, 0x03, 0x12, 0xE1);

#define DEFINE_POLICY(policy, appstr, keystr) \
    { policy, L##appstr, L##keystr }

static const POLICYDATA s_PolicyTable[] =
{
#include "PolicyData.h"
    { 0, NULL, NULL }
};

#undef DEFINE_POLICY

/*
 * The restriction-related variables
 */
HANDLE g_hRestGlobalCounter = NULL;
LONG g_nRestCountValue = -1;
DWORD g_RestValues[_countof(s_PolicyTable)] = { 0 };

/****************************************************************************
 *                  SHELL_GetCachedGlobalCounter
 *
 * Retrieves the global counter using cache in a thread-safe manner.
 * If a cache of global counter exists, the function returns it.
 * If there is no cache, the function creates a global counter.
 *
 * @param[in,out]  phGlobalCounter  The pointer to the handle of global counter.
 * @param[in]      rguid            The GUID of global counter.
 * @return  The handle of the global counter.
 * @implemented
 */
#ifdef __REACTOS__
EXTERN_C HANDLE
#else
static HANDLE
#endif
SHELL_GetCachedGlobalCounter(_Inout_ HANDLE *phGlobalCounter, _In_ REFGUID rguid)
{
    HANDLE hGlobalCounter;
    if (*phGlobalCounter)
        return *phGlobalCounter;
    hGlobalCounter = SHGlobalCounterCreate(rguid);
    if (InterlockedCompareExchangePointer(phGlobalCounter, hGlobalCounter, NULL))
        CloseHandle(hGlobalCounter);
    return *phGlobalCounter;
}

/****************************************************************************
 *                  SHELL_GetRestrictionsCounter
 *
 * Retrieves the global counter for GUID_Restrictions using caching in a
 * thread-safe manner. The variable g_hRestGlobalCounter is used for caching.
 *
 * @return  The handle of the global counter.
 * @see SHELL_GetCachedGlobalCounter
 * @implemented
 */
static HANDLE SHELL_GetRestrictionsCounter(VOID)
{
    return SHELL_GetCachedGlobalCounter(&g_hRestGlobalCounter, &GUID_Restrictions);
}

/****************************************************************************
 *                  SHELL_QueryRestrictionsChanged
 *
 * @return  The value of the global counter for GUID_Restrictions.
 * @see SHELL_GetRestrictionsCounter
 * @implemented
 */
static BOOL SHELL_QueryRestrictionsChanged(VOID)
{
    LONG Value = SHGlobalCounterGetValue(SHELL_GetRestrictionsCounter());
    if (g_nRestCountValue == Value)
        return FALSE;

    g_nRestCountValue = Value;
    return TRUE;
}

/*************************************************************************
 * SHRestricted				 [SHELL32.100]
 *
 * Get the value associated with a policy Id.
 *
 * PARAMS
 *     pol [I] Policy Id
 *
 * RETURNS
 *     The queried value for the policy.
 *
 * NOTES
 *     Exported by ordinal.
 *     This function caches the retrieved values to prevent unnecessary registry access,
 *     if SHSettingsChanged() was previously called.
 *
 * REFERENCES
 *     a: MS System Policy Editor.
 *     b: 98Lite 2.0 (which uses many of these policy keys) http://www.98lite.net/
 *     c: 'The Windows 95 Registry', by John Woram, 1996 MIS: Press
 */
DWORD WINAPI SHRestricted (RESTRICTIONS rest)
{
    TRACE("(0x%08lX)\n", rest);

    /* If restrictions from registry have changed, reset all cached values to SHELL_NO_POLICY */
    if (SHELL_QueryRestrictionsChanged())
        FillMemory(&g_RestValues, sizeof(g_RestValues), 0xFF);

    return SHRestrictionLookup(rest, NULL, s_PolicyTable, g_RestValues);
}

/*************************************************************************
 * SHSettingsChanged          [SHELL32.244]
 *
 * Initialise the policy cache to speed up calls to SHRestricted().
 *
 * PARAMS
 *  unused    [I] Reserved.
 *  pszKey    [I] Registry key to scan.
 *
 * RETURNS
 *  Success: -1. The policy cache is initialised.
 *  Failure: 0, if inpRegKey is any value other than NULL, "Policy", or
 *           "Software\Microsoft\Windows\CurrentVersion\Policies".
 *
 * NOTES
 *  Exported by ordinal. Introduced in Win98.
 */
BOOL WINAPI SHSettingsChanged(LPCVOID unused, LPCWSTR pszKey)
{
    TRACE("(%p, %s)\n", unused, debugstr_w(pszKey));

    if (pszKey &&
        lstrcmpiW(L"Policy", pszKey) != 0 &&
        lstrcmpiW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies", pszKey) != 0)
    {
        return FALSE;
    }

    return SHGlobalCounterIncrement(SHELL_GetRestrictionsCounter());
}














#define IUnknown_AddRef(This) (This)->lpVtbl->AddRef(This)


struct window_prop_store
{
    IPropertyStore IPropertyStore_iface;
    LONG           ref;
};

static inline struct window_prop_store *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, struct window_prop_store, IPropertyStore_iface);
}

static ULONG WINAPI window_prop_store_AddRef(IPropertyStore *iface)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedIncrement(&store->ref);
    TRACE("returning %ld\n", ref);
    return ref;
}

static ULONG WINAPI window_prop_store_Release(IPropertyStore *iface)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedDecrement(&store->ref);
    if (!ref) free(store);
    TRACE("returning %ld\n", ref);
    return ref;
}

static HRESULT WINAPI window_prop_store_QueryInterface(IPropertyStore *iface, REFIID iid, void **obj)
{
    struct window_prop_store *store = impl_from_IPropertyStore(iface);
    TRACE("%p, %s, %p\n", iface, debugstr_guid(iid), obj);

    if (!obj) return E_POINTER;
    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IPropertyStore))
    {
        *obj = &store->IPropertyStore_iface;
    }
    else
    {
        FIXME("no interface for %s\n", debugstr_guid(iid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static HRESULT WINAPI window_prop_store_GetCount(IPropertyStore *iface, DWORD *count)
{
    FIXME("%p, %p\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_GetAt(IPropertyStore *iface, DWORD prop, PROPERTYKEY *key)
{
    FIXME("%p, %lu,%p\n", iface, prop, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_GetValue(IPropertyStore *iface, const PROPERTYKEY *key, PROPVARIANT *var)
{
    FIXME("%p, {%s,%lu}, %p\n", iface, debugstr_guid(&key->fmtid), key->pid, var);
    return E_NOTIMPL;
}

static HRESULT WINAPI window_prop_store_SetValue(IPropertyStore *iface, const PROPERTYKEY *key, const PROPVARIANT *var)
{
    FIXME("%p, {%s,%lu}, %p\n", iface, debugstr_guid(&key->fmtid), key->pid, var);
    return S_OK;
}

static HRESULT WINAPI window_prop_store_Commit(IPropertyStore *iface)
{
    FIXME("%p\n", iface);
    return S_OK;
}

static IPropertyStoreVtbl window_prop_store_vtbl =
{
    window_prop_store_QueryInterface,
    window_prop_store_AddRef,
    window_prop_store_Release,
    window_prop_store_GetCount,
    window_prop_store_GetAt,
    window_prop_store_GetValue,
    window_prop_store_SetValue,
    window_prop_store_Commit
};

static HRESULT create_window_prop_store(IPropertyStore **obj)
{
    struct window_prop_store *store;

    if (!(store = malloc(sizeof(*store)))) return E_OUTOFMEMORY;
    store->IPropertyStore_iface.lpVtbl = &window_prop_store_vtbl;
    store->ref = 1;

    *obj = &store->IPropertyStore_iface;
    return S_OK;
}
/*** IUnknown methods ***/
FORCEINLINE HRESULT IPropertyStore_QueryInterface(IPropertyStore* This,REFIID riid,void **ppvObject) {
    return This->lpVtbl->QueryInterface(This,riid,ppvObject);
}
FORCEINLINE ULONG IPropertyStore_AddRef(IPropertyStore* This) {
    return This->lpVtbl->AddRef(This);
}
FORCEINLINE ULONG IPropertyStore_Release(IPropertyStore* This) {
    return This->lpVtbl->Release(This);
}
/*************************************************************************
 * SHGetPropertyStoreForWindow [SHELL32.@]
 */
HRESULT WINAPI SHGetPropertyStoreForWindow(HWND hwnd, REFIID riid, void **ppv)
{
    IPropertyStore *store;
    HRESULT hr;

    FIXME("(%p %p %p) stub!\n", hwnd, riid, ppv);

    if ((hr = create_window_prop_store( &store )) != S_OK) return hr;
    hr = IPropertyStore_QueryInterface( store, riid, ppv );
    IPropertyStore_Release( store );
    return hr;
}


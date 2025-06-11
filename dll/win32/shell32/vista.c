/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            Copied from advapi32/reg/reg.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  19990309 EA Stubs
 *                  20050502 Fireball imported some stuff from WINE
 */

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/******************************************************************************
 * load_string [Internal]
 *
 * This is basically a copy of user32/resource.c's LoadStringW. Necessary to
 * avoid importing user32, which is higher level than advapi32. Helper for
 * RegLoadMUIString.
 */
static int load_string(HINSTANCE hModule, UINT resId, LPWSTR pwszBuffer, INT cMaxChars)
{
    HGLOBAL hMemory;
    HRSRC hResource;
    WCHAR *pString;
    int idxString;

    /* Negative values have to be inverted. */
    if (HIWORD(resId) == 0xffff)
        resId = (UINT)(-((INT)resId));

    /* Load the resource into memory and get a pointer to it. */
    hResource = FindResourceW(hModule, MAKEINTRESOURCEW(LOWORD(resId >> 4) + 1), (LPWSTR)RT_STRING);
    if (!hResource) return 0;
    hMemory = LoadResource(hModule, hResource);
    if (!hMemory) return 0;
    pString = LockResource(hMemory);

    /* Strings are length-prefixed. Lowest nibble of resId is an index. */
    idxString = resId & 0xf;
    while (idxString--) pString += *pString + 1;

    /* If no buffer is given, return length of the string. */
    if (!pwszBuffer) return *pString;

    /* Else copy over the string, respecting the buffer size. */
    cMaxChars = (*pString < cMaxChars) ? *pString : (cMaxChars - 1);
    if (cMaxChars >= 0)
    {
        memcpy(pwszBuffer, pString+1, cMaxChars * sizeof(WCHAR));
        pwszBuffer[cMaxChars] = L'\0';
    }

    return cMaxChars;
}

/************************************************************************
 *  RegLoadMUIStringW
 *
 * @implemented
 */
LONG WINAPI
RegLoadMUIStringW(IN HKEY hKey,
                  IN LPCWSTR pszValue  OPTIONAL,
                  OUT LPWSTR pszOutBuf,
                  IN DWORD cbOutBuf,
                  OUT LPDWORD pcbData OPTIONAL,
                  IN DWORD Flags,
                  IN LPCWSTR pszDirectory  OPTIONAL)
{
    DWORD dwValueType, cbData;
    LPWSTR pwszTempBuffer = NULL, pwszExpandedBuffer = NULL;
    LONG result;

    /* Parameter sanity checks. */
    if (!hKey || !pszOutBuf)
        return ERROR_INVALID_PARAMETER;

    if (pszDirectory && *pszDirectory)
    {
        FIXME("BaseDir parameter not yet supported!\n");
        return ERROR_INVALID_PARAMETER;
    }

    /* Check for value existence and correctness of it's type, allocate a buffer and load it. */
    result = RegQueryValueExW(hKey, pszValue, NULL, &dwValueType, NULL, &cbData);
    if (result != ERROR_SUCCESS) goto cleanup;
    if (!(dwValueType == REG_SZ || dwValueType == REG_EXPAND_SZ) || !cbData)
    {
        result = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }
    pwszTempBuffer = HeapAlloc(GetProcessHeap(), 0, cbData);
    if (!pwszTempBuffer)
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    result = RegQueryValueExW(hKey, pszValue, NULL, &dwValueType, (LPBYTE)pwszTempBuffer, &cbData);
    if (result != ERROR_SUCCESS) goto cleanup;

    /* Expand environment variables, if appropriate, or copy the original string over. */
    if (dwValueType == REG_EXPAND_SZ)
    {
        cbData = ExpandEnvironmentStringsW(pwszTempBuffer, NULL, 0) * sizeof(WCHAR);
        if (!cbData) goto cleanup;
        pwszExpandedBuffer = HeapAlloc(GetProcessHeap(), 0, cbData);
        if (!pwszExpandedBuffer)
        {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        ExpandEnvironmentStringsW(pwszTempBuffer, pwszExpandedBuffer, cbData);
    }
    else
    {
        pwszExpandedBuffer = HeapAlloc(GetProcessHeap(), 0, cbData);
        memcpy(pwszExpandedBuffer, pwszTempBuffer, cbData);
    }

    /* If the value references a resource based string, parse the value and load the string.
     * Else just copy over the original value. */
    result = ERROR_SUCCESS;
    if (*pwszExpandedBuffer != L'@') /* '@' is the prefix for resource based string entries. */
    {
        lstrcpynW(pszOutBuf, pwszExpandedBuffer, cbOutBuf / sizeof(WCHAR));
    }
    else
    {
        WCHAR *pComma = wcsrchr(pwszExpandedBuffer, L',');
        UINT uiStringId;
        HMODULE hModule;

        /* Format of the expanded value is 'path_to_dll,-resId' */
        if (!pComma || pComma[1] != L'-')
        {
            result = ERROR_BADKEY;
            goto cleanup;
        }

        uiStringId = _wtoi(pComma+2);
        *pComma = L'\0';

        hModule = LoadLibraryExW(pwszExpandedBuffer + 1, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!hModule || !load_string(hModule, uiStringId, pszOutBuf, cbOutBuf / sizeof(WCHAR)))
            result = ERROR_BADKEY;
        FreeLibrary(hModule);
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, pwszTempBuffer);
    HeapFree(GetProcessHeap(), 0, pwszExpandedBuffer);
    return result;
}

/************************************************************************
 *  RegLoadMUIStringA
 *
 * @implemented
 */
LONG WINAPI
RegLoadMUIStringA(IN HKEY hKey,
                  IN LPCSTR pszValue  OPTIONAL,
                  OUT LPSTR pszOutBuf,
                  IN DWORD cbOutBuf,
                  OUT LPDWORD pcbData OPTIONAL,
                  IN DWORD Flags,
                  IN LPCSTR pszDirectory  OPTIONAL)
{
    UNICODE_STRING valueW, baseDirW;
    WCHAR *pwszBuffer;
    DWORD cbData = cbOutBuf * sizeof(WCHAR);
    LONG result;

    valueW.Buffer = baseDirW.Buffer = pwszBuffer = NULL;
    if (!RtlCreateUnicodeStringFromAsciiz(&valueW, pszValue) ||
        !RtlCreateUnicodeStringFromAsciiz(&baseDirW, pszDirectory) ||
        !(pwszBuffer = HeapAlloc(GetProcessHeap(), 0, cbData)))
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    result = RegLoadMUIStringW(hKey, valueW.Buffer, pwszBuffer, cbData, NULL, Flags,
                               baseDirW.Buffer);

    if (result == ERROR_SUCCESS)
    {
        cbData = WideCharToMultiByte(CP_ACP, 0, pwszBuffer, -1, pszOutBuf, cbOutBuf, NULL, NULL);
        if (pcbData)
            *pcbData = cbData;
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, pwszBuffer);
    RtlFreeUnicodeString(&baseDirW);
    RtlFreeUnicodeString(&valueW);

    return result;
}


/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
 */

 

#include <ole2.h>
#include <olectl.h>
 
 
#include <propsys.h>

 
static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

FORCEINLINE HRESULT IPropertyStore_QueryInterface(IPropertyStore* This,REFIID riid,void **ppvObject) {
    return This->lpVtbl->QueryInterface(This,riid,ppvObject);
}
FORCEINLINE ULONG IPropertyStore_AddRef(IPropertyStore* This) {
    return This->lpVtbl->AddRef(This);
}
FORCEINLINE ULONG IPropertyStore_Release(IPropertyStore* This) {
    return This->lpVtbl->Release(This);
}

typedef struct _NOTIFYICONIDENTIFIER {
  DWORD cbSize;
  HWND  hWnd;
  UINT  uID;
  GUID  guidItem;
} NOTIFYICONIDENTIFIER, *PNOTIFYICONIDENTIFIER;

typedef enum _ASSOCCLASS{
	ASSOCCLASS_APP_KEY,
	ASSOCCLASS_CLSID_KEY,
	ASSOCCLASS_CLSID_STR,
	ASSOCCLASS_PROGID_KEY,
	ASSOCCLASS_SHELL_KEY,
	ASSOCCLASS_PROGID_STR,
	ASSOCCLASS_SYSTEM_STR,
	ASSOCCLASS_APP_STR,
	ASSOCCLASS_FOLDER,
	ASSOCCLASS_STAR,
	ASSOCCLASS_FIXED_PROGID_STR,
	ASSOCCLASS_PROTOCOL_STR,
}ASSOCCLASS;

typedef enum tagSCNRT_STATUS { 
  SCNRT_ENABLE   = 0,
  SCNRT_DISABLE  = 1
} SCNRT_STATUS;

typedef struct _ASSOCIATIONELEMENT {
  ASSOCCLASS ac;
  HKEY       hkClass;
  PCWSTR     pszClass;
} ASSOCIATIONELEMENT;

struct window_prop_store
{
    IPropertyStore IPropertyStore_iface;
    LONG           ref;
};
#define IUnknown_AddRef(T) (T)->lpVtbl->AddRef(T)
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
    if (!ref) heap_free(store);
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

    if (!(store = heap_alloc(sizeof(*store)))) return E_OUTOFMEMORY;
    store->IPropertyStore_iface.lpVtbl = &window_prop_store_vtbl;
    store->ref = 1;

    *obj = &store->IPropertyStore_iface;
    return S_OK;
}

HRESULT 
WINAPI 
SHGetPropertyStoreForWindow(
  _In_   HWND hwnd,
  _In_   REFIID riid,
  _Out_  void **ppv
)
{
    IPropertyStore *store;
    HRESULT hr;

    FIXME("(%p %p %p) stub!\n", hwnd, riid, ppv);

    if ((hr = create_window_prop_store( &store )) != S_OK) return hr;
    hr = IPropertyStore_QueryInterface( store, riid, ppv );
    IPropertyStore_Release( store );
    return hr;
}
 
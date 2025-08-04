
#include <hstring.h>
#include <winstring.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION


#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include <roapi.h>

#include "ole2.h"
#include "oleauto.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);


#define HSTRING_REFERENCE_FLAG 1

struct hstring_header
{
    UINT32 flags;
    UINT32 length;
    UINT32 padding1;
    UINT32 padding2;
    const WCHAR *str;
};

struct hstring_private
{
    struct hstring_header header;
    LONG refcount;
    WCHAR buffer[1];
};
static const WCHAR empty[1];

C_ASSERT(sizeof(struct hstring_header) <= sizeof(HSTRING_HEADER));

static inline struct hstring_private *impl_from_HSTRING(HSTRING string)
{
    return (struct hstring_private *)string;
}

static inline struct hstring_private *impl_from_HSTRING_HEADER(HSTRING_HEADER *header)
{
    return CONTAINING_RECORD(header, struct hstring_private, header);
}

static inline struct hstring_private *impl_from_HSTRING_BUFFER(HSTRING_BUFFER buffer)
{
    return CONTAINING_RECORD(buffer, struct hstring_private, buffer);
}



static inline const char *wine_dbgstr_hstring( HSTRING hstr )
{
    UINT32 len;
    const WCHAR *str = WindowsGetStringRawBuffer( hstr, &len );
    return wine_dbgstr_wn( str, len );
}

static inline const char *debugstr_hstring( struct HSTRING__ *s ) { return wine_dbgstr_hstring( s ); }

/***********************************************************************
 *      WindowsGetStringRawBuffer (combase.@)
 */
LPCWSTR WINAPI WindowsGetStringRawBuffer(HSTRING str, UINT32 *len)
{
    struct hstring_private *priv = impl_from_HSTRING(str);

    TRACE("(%p, %p)\n", str, len);

    if (str == NULL)
    {
        if (len)
            *len = 0;
        return empty;
    }
    if (len)
        *len = priv->header.length;
    return priv->header.str;
}

/***********************************************************************
 *      RoOriginateLanguageException (combase.@)
 */
BOOL WINAPI RoOriginateLanguageException(HRESULT error, HSTRING message, IUnknown *language_exception)
{
    FIXME("%#lx, %s, %p: stub\n", error, debugstr_hstring(message), language_exception);
    return FALSE;
}

/***********************************************************************
 *      RoOriginateError (combase.@)
 */
BOOL WINAPI RoOriginateError(HRESULT error, HSTRING message)
{
    FIXME("%#lx, %s: stub\n", error, debugstr_hstring(message));
    return FALSE;
}

/***********************************************************************
 *      RoOriginateErrorW (combase.@)
 */
BOOL WINAPI RoOriginateErrorW(HRESULT error, UINT max_len, const WCHAR *message)
{
    FIXME("%#lx, %u, %p: stub\n", error, max_len, message);
    return FALSE;
}


struct activatable_class_data
{
    ULONG size;
    DWORD unk;
    DWORD module_len;
    DWORD module_offset;
    DWORD threading_model;
};

static HRESULT get_library_for_classid(const WCHAR *classid, WCHAR **out)
{
    ACTCTX_SECTION_KEYED_DATA data;
    HKEY hkey_root, hkey_class;
    DWORD type, size;
    HRESULT hr;
    WCHAR *buf = NULL;

    *out = NULL;

    /* search activation context first */
    data.cbSize = sizeof(data);
    if (FindActCtxSectionStringW(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
            ACTIVATION_CONTEXT_SECTION_WINRT_ACTIVATABLE_CLASSES, classid, &data))
    {
        struct activatable_class_data *activatable_class = (struct activatable_class_data *)data.lpData;
        void *ptr = (BYTE *)data.lpSectionBase + activatable_class->module_offset;
        *out = wcsdup(ptr);
        return S_OK;
    }

    /* load class registry key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\WindowsRuntime\\ActivatableClassId",
                      0, KEY_READ, &hkey_root))
        return REGDB_E_READREGDB;
    if (RegOpenKeyExW(hkey_root, classid, 0, KEY_READ, &hkey_class))
    {
        WARN("Class %s not found in registry\n", debugstr_w(classid));
        RegCloseKey(hkey_root);
        return REGDB_E_CLASSNOTREG;
    }
    RegCloseKey(hkey_root);

    /* load (and expand) DllPath registry value */
    if (RegQueryValueExW(hkey_class, L"DllPath", NULL, &type, NULL, &size))
    {
        hr = REGDB_E_READREGDB;
        goto done;
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ)
    {
        hr = REGDB_E_READREGDB;
        goto done;
    }
    if (!(buf = malloc(size)))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    if (RegQueryValueExW(hkey_class, L"DllPath", NULL, NULL, (BYTE *)buf, &size))
    {
        hr = REGDB_E_READREGDB;
        goto done;
    }
    if (type == REG_EXPAND_SZ)
    {
        WCHAR *expanded;
        DWORD len = ExpandEnvironmentStringsW(buf, NULL, 0);
        if (!(expanded = malloc(len * sizeof(WCHAR))))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        ExpandEnvironmentStringsW(buf, expanded, len);
        free(buf);
        buf = expanded;
    }

    *out = buf;
    return S_OK;

done:
    free(buf);
    RegCloseKey(hkey_class);
    return hr;
}



HRESULT WINAPI RoGetActivationFactory(HSTRING classid, REFIID iid, void **class_factory)
{
   PFNGETACTIVATIONFACTORY pDllGetActivationFactory;
    IActivationFactory *factory;
    WCHAR *library;
    HMODULE module;
    HRESULT hr;

        FIXME("(%s, %s, %p): semi-stub\n", debugstr_hstring(classid), debugstr_guid(iid), class_factory);

         if (!iid || !class_factory)
        return E_INVALIDARG;

    hr = get_library_for_classid(WindowsGetStringRawBuffer(classid, NULL), &library);
    if (FAILED(hr))
    {
        ERR("Failed to find library for %s\n", debugstr_hstring(classid));
        return hr;
    }

    if (!(module = LoadLibraryW(library)))
    {
        ERR("Failed to load module %s\n", debugstr_w(library));
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    if (!(pDllGetActivationFactory = (void *)GetProcAddress(module, "DllGetActivationFactory")))
    {
        ERR("Module %s does not implement DllGetActivationFactory\n", debugstr_w(library));
        hr = E_FAIL;
        goto done;
    }

    TRACE("Found library %s for class %s\n", debugstr_w(library), debugstr_hstring(classid));

    hr = pDllGetActivationFactory(classid, &factory);
    if (SUCCEEDED(hr))
    {
        hr = IActivationFactory_QueryInterface(factory, iid, class_factory);
        if (SUCCEEDED(hr))
        {
            TRACE("Created interface %p\n", *class_factory);
            module = NULL;
        }
        IActivationFactory_Release(factory);
    }
    else
    {
        ERR("Class %s not found in %s, hr %#lx.\n", wine_dbgstr_hstring(classid), debugstr_w(library), hr);
    }

done:
    free(library);
    if (module) FreeLibrary(module);
    return hr;
}

HRESULT WINAPI RoGetAgileReference(enum AgileReferenceOptions option, REFIID riid, IUnknown *obj,
                                   IAgileReference **agile_reference)
                                   {
                                    return E_NOTIMPL;
                                   }
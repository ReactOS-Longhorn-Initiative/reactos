#pragma once

#include <initguid.h>
#include <windef.h>
#include <winbase.h>
#include <oleauto.h>
#include <olectl.h>
#include <cguid.h>
#include <tchar.h>
#include <strsafe.h>
#include <mmdeviceapi.h>

#include <wine/list.h>
#include <wine/debug.h>

// Imports
EXTERN_C HRESULT __wine_register_resources(HMODULE module);
EXTERN_C HRESULT __wine_unregister_resources(HMODULE module);

// DeviceEnumerator.cpp
EXTERN_C HRESULT MMDeviceEnumerator_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut);

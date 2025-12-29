// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  File:   rsapi.cpp
//
//  Description:
//      Implementation of the graphics stream APIs for accessibility. 
//

#include "precomp.hpp"

// Compatibility for Vista dwm api.
typedef MilMatrix3x2D MIL_MATRIX3X2D;


typedef HRESULT (WINAPI *PFNDWMGETGRAPHICSSTREAMTRANSFORMHINT)(
    _In_ UINT uIndex,
    __out_ecount(1) MilMatrix3x2D *pTransform
    );

typedef HRESULT (WINAPI *PFNDWMGETGRAPHICSSTREAMCLIENT)(
    _In_ UINT uIndex,
    __out_ecount(1) UUID *pClientUuid
    );

typedef HRESULT (WINAPI *PFNDWMPATTACHMILCONTENT)(HWND hwnd);
typedef HRESULT (WINAPI *PFNDWMPDETACHMILCONTENT)(HWND hwnd);

//
// Critical section to synchronize access to the graphics stream globals
// 

CCriticalSection g_csGraphicsStream;

EXTERN_C
HRESULT
WINAPI
MilGraphicsStream_Close(PVOID Adapter)
{
    __debugbreak();
    return S_OK;
}

EXTERN_C
HRESULT
WINAPI
MilGraphicsStream_Open(PVOID MilConnectionManager, PVOID MILGraphicsStreamClien, MIL_MATRIX3X2D* pTransform)
{
    __debugbreak();
    return S_OK;
}

EXTERN_C
HRESULT
WINAPI
MilGraphicsStream_SetTransformHint(MIL_MATRIX3X2D* pTransform)
{
    __debugbreak();
    return S_OK;
}

EXTERN_C
HRESULT
WINAPI
MilGraphicsStream_GetTransformHint(
    _In_ UINT uIndex,
    __out_ecount(1) MilMatrix3x2D *pTransform
    )
{
    HRESULT hr = S_OK;
{
    //
    // Don't break on E_INVALIDARG -- this error code is used to report that
    // there are no more graphics streams to be enumerated.
    //

    BEGIN_MILINSTRUMENTATION_HRESULT_LIST
        E_INVALIDARG
    END_MILINSTRUMENTATION_HRESULT_LIST

    //
    // Do not attempt to load dwmapi.dll on down-level platforms.
    //
    if (!DWMAPI::CheckOS())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    //  
    // Call the DWM to enumerate graphics streams.
    //
    IFC(DWMAPI::Load());
    PFNDWMGETGRAPHICSSTREAMTRANSFORMHINT pfnGetGraphicsStreamTransformHint = NULL;
    IFCW32(pfnGetGraphicsStreamTransformHint = 
           reinterpret_cast<PFNDWMGETGRAPHICSSTREAMTRANSFORMHINT>(
               DWMAPI::GetProcAddress(
                   "DwmGetGraphicsStreamTransformHint"
                   )));
    IFC(pfnGetGraphicsStreamTransformHint(
            uIndex,
            pTransform
            ));
        }
Cleanup:
    if (FAILED(hr) && hr != E_INVALIDARG)
    {   
        TraceTag((tagMILWarning, 
                  "MilGraphicsStream_Enum: failed with HRESULT 0x%08x", 
                  hr
                  ));
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//    Function: 
//        GetGraphicsStreamClient
//
//    Synopsis:
//        Enumerates graphics stream clients registered with 
//        the current session.
//
//------------------------------------------------------------------------------

HRESULT
GetGraphicsStreamClient(
    _In_ UINT uIndex,
    __out_ecount(1) UUID* pUuid
    )
{
    HRESULT hr = S_OK;
{
    //
    // Don't break on E_INVALIDARG -- this error code is used to report that
    // there are no more graphics streams to be enumerated.
    //

    BEGIN_MILINSTRUMENTATION_HRESULT_LIST
        E_INVALIDARG
    END_MILINSTRUMENTATION_HRESULT_LIST


    //
    // Do not attempt to load dwmapi.dll on down-level platforms.
    //

    if (!DWMAPI::CheckOS())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    //
    // Call the DWM to enumerate graphics streams.
    //
    
    IFC(DWMAPI::Load());
    
    PFNDWMGETGRAPHICSSTREAMCLIENT pfnGetGraphicsStreamClient = NULL;
    
    IFCW32(pfnGetGraphicsStreamClient = 
           reinterpret_cast<PFNDWMGETGRAPHICSSTREAMCLIENT>(
               DWMAPI::GetProcAddress(
                   "DwmGetGraphicsStreamClient"
                   )));
    
    IFC(pfnGetGraphicsStreamClient(
            uIndex,
            pUuid
            ));
        }
Cleanup:
    if (FAILED(hr) && hr != E_INVALIDARG)
    {   
        TraceTag((tagMILWarning, 
                  "MilGraphicsStream_Enum: failed with HRESULT 0x%08x", 
                  hr
                  ));
    }
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method: MilGraphicsContent_AttachToHwnd
//
//------------------------------------------------------------------------------
EXTERN_C HRESULT WINAPI 
MilContent_AttachToHwnd(HWND hwnd)
{
    HRESULT hr = S_OK;
    
    //
    // We send hint only if we are not running on the downlevel platform. 
    // Otherwise, succeed and return.
    //
    if (DWMAPI::CheckOS())
    {
        //
        // Hint the DWM about the presence of MIL content in this window
        //
        
        if (SUCCEEDED(DWMAPI::Load()))
        {
            PFNDWMPATTACHMILCONTENT pfnAttachMilContent = 
                reinterpret_cast<PFNDWMPATTACHMILCONTENT>(DWMAPI::GetProcAddress(
                    "DwmAttachMilContent"
                    ));
            
            if (pfnAttachMilContent)
            {
                IGNORE_HR((*pfnAttachMilContent)(hwnd));
            }
        }    
    }
   
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method: MilGraphicsContent_DetachFromHwnd
//
//------------------------------------------------------------------------------
EXTERN_C HRESULT WINAPI 
MilContent_DetachFromHwnd(HWND hwnd)
{
    HRESULT hr = S_OK;
    
    //
    // We send hint only if we are not running on the downlevel platform. 
    // Otherwise, succeed and return.
    //
    if (DWMAPI::CheckOS())
    {
        //
        // Hint the DWM about the absence of MIL content in this window
        //

        if (SUCCEEDED(DWMAPI::Load()))
        {
            PFNDWMPDETACHMILCONTENT pfnDetachMilContent = 
                reinterpret_cast<PFNDWMPDETACHMILCONTENT>(DWMAPI::GetProcAddress(
                    "DwmDetachMilContent"
                    ));
            
            if (pfnDetachMilContent)
            {
                IGNORE_HR((*pfnDetachMilContent)(hwnd));
            }
        }    
    }
    
    RRETURN(hr);
}





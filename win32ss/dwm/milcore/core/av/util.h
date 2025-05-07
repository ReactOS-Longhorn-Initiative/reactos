// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_media
//      $Keywords:
//
//  $Description:
//      Provides for simple utility functions. The general rule is that none of
//      the functions in this file can have dependencies on other functions in
//      the file. If this rule is broken, seperate out the functions into their
//      own file.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#pragma once

inline
HRESULT
IsSupportedWmpReturn(
    _In_    HRESULT         hr
    );

HRESULT
GetLastErrorAsFailHR(
    void
    );

DWORD
Win32StatusFromHR(
    _In_    HRESULT         hr
    );

inline
HRESULT
SysAllocStringCheck(
    _In_    PCWSTR          pszString,
    _Out_   BSTR            *pbstrString
    );

HRESULT
CopyHeapString(
    __in_opt    PCWSTR          stringIn,
    __deref_out PWSTR           *pStringOut
    );

void
GetUnderlyingDevice(
    _In_        CD3DDeviceLevel1    *pCD3DDeviceLevel1,
    __deref_out IDirect3DDevice9    **ppIDirect3DDevice9
    );

class CMFMediaBuffer;

HRESULT
ConvertSampleToMediaBuffer(
    _In_    IMFSample       *pIMFSample,
    _Out_   CMFMediaBuffer  **ppCMFMediaBuffer
    );

inline
D3DFORMAT
FormatFromMediaType(
    _In_        IMFVideoMediaType   *pIVideoMediaType
    );

#ifndef COUNTOF
#   define COUNTOF(x)   (sizeof(x)/(sizeof(*(x))))
#endif

template<class T>
void
SmartRelease(
    T **ppInstance
    );

#define IFCN(expr)                                                  \
        do  {                                                       \
            hr = (expr); /* Evaluate expr here*/               \
            if (FAILED(hr)) {                                       \
                goto Cleanup;                                       \
            }                                                       \
        } while (UNCONDITIONAL_EXPR(0))                             \


#include "util.inl"


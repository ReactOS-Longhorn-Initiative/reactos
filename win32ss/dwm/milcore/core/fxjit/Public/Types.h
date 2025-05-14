// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Platform independent data types and macros (rough)
//
//-----------------------------------------------------------------------------
#pragma once

#if !defined(_BASETSD_H_)
#include <basetsd.h>
typedef int BOOL;

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
#undef size_t
#ifdef _WIN64
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
  typedef unsigned int size_t __attribute__ ((mode (DI)));
#else
  typedef unsigned __int64 size_t;
#endif
#else
  typedef unsigned int size_t;
#endif
#endif

#define NULL    0

#define FALSE   0
#define TRUE    1

#define UINT_MAX                    0xffffffff
#define UNREFERENCED_PARAMETER(x)   (x)

#ifndef HRESULT
#define HRESULT long
#endif

#ifndef S_OK
#define S_OK                        HRESULT(0x00000000L)
#endif

#ifndef S_FALSE
#define S_FALSE                     HRESULT(0x00000001L)
#endif

#ifndef E_OUTOFMEMORY
#define E_OUTOFMEMORY               HRESULT(0x8007000EL)
#endif

#ifndef E_FAIL
#define E_FAIL                      HRESULT(0x80004005L)
#endif

#ifndef FAILED
#define FAILED(hr)  (((HRESULT)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#endif

#ifndef RRETURN
#define RRETURN(hr) return (hr)
#endif

#ifndef IFC
#define IFC(x) { hr = (x); if (FAILED(hr)) goto Cleanup; }
#endif

#ifndef IFCOOM
#define IFCOOM(x) if ((x) == NULL) {hr = E_OUTOFMEMORY; goto Cleanup;}
#endif

#ifndef ReleaseInterface
#define ReleaseInterface(p) if ((p)) {(p)->Release(); (p) = NULL;}
#endif

#endif //_BASETSD_H_

#ifndef __STDCALL
#define __STDCALL  __stdcall
#endif



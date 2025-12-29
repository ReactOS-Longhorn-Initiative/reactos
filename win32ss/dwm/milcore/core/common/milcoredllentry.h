// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      MILCore.dll entry point header
//
//-----------------------------------------------------------------------------

#pragma once

BOOL
MILCoreDllMain(
    _In_ HINSTANCE   dllHandle,
    ULONG       reason
    );

STDAPI
MILCoreDllCanUnloadNow(
    void
    );

STDAPI
MILCoreDllGetClassObject(
    _In_        REFCLSID        clsid,
    _In_        REFIID          riid,
    __deref_out void            **ppv
    );


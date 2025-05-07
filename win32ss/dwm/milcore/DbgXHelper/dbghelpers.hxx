// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// File: helpers.hxx
//---------------------------------------------------------------------------------

void OutputInstance(
    PDEBUG_CLIENT Client,
    PCSTR typeName,
    ULONG64 typeAddress,
    bool fVerbose
    );

HRESULT GetFieldOffset(
    PDEBUG_CLIENT Client,
    PCSTR typeName,
    PCSTR fieldName,
    _Out_ ULONG* pFieldOffset
    );

HRESULT ReadPointerField(
    PDEBUG_CLIENT Client,
    ULONG64 typeAddress,
    PCSTR typeName,
    PCSTR fieldName,
    _Out_ ULONG64* pFieldValue
    );

HRESULT ReadNonPointerField(
    PDEBUG_CLIENT Client,
    ULONG64 typeAddress,
    PCSTR typeName,
    PCSTR fieldName,
    ULONG fieldSize,
    _Out_ VOID* pFieldValue
    );

template<typename T>
HRESULT ReadTypedField(
    PDEBUG_CLIENT Client,
    ULONG64 typeAddress,
    PCSTR typeName,
    PCSTR fieldName,
    _Out_ T* pFieldValue
    )
{
    return ReadNonPointerField(Client, typeAddress, typeName, fieldName, sizeof(*pFieldValue), pFieldValue);
}

HRESULT ReadSymbolNameByOffset(
    PDEBUG_CLIENT client,
    ULONG64 offset,
    ULONG nameBufferSize,
    __out_ecount_part(nameBufferSize, *pNameSize) PSTR nameBuffer,
    __out_ecount(1) ULONG *pNameSize
    );

HRESULT SearchTable(
    PDEBUG_CLIENT Client,
    ULONG64 ulpTableRoot, 
    ULONG ulFieldOffset, 
    ULONG64 ulValueToLookFor,
    _Out_ ULONG64* ulpEntry
    );




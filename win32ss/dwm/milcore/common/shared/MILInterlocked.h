// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Header for InterlockedExchange64 replacement
//
//-----------------------------------------------------------------------------

#pragma once

//
// Function Prototypes
//


//+----------------------------------------------------------------------------
//
//  Member:    MILInterlockedAvailable
//
//  Synopsis:  Returns whether the processor is capable of performing
//             MILInterlockedCompareExchange64
//
//-----------------------------------------------------------------------------
BOOL MILInterlockedAvailable();

//+----------------------------------------------------------------------------
//
//  Member:    MILInterlockedCompareExchange64
//
//  Synopsis:  Performs an atomic operation that does the following...
//             a) if (*pDestination == comperand)
//                   *pDestination = exchange
//             b) returns the value of *pDestination before the operation
//
//-----------------------------------------------------------------------------
__int64 MILInterlockedCompareExchange64(
    __inout_ecount(1) __int64 volatile * pDestination, 
    __int64 exchange, 
    __int64 comperand
    );

//
// Implementation
//

#ifdef _X86_

//
// X86 Implementation- assembly
//

//+----------------------------------------------------------------------------
//
//  Member:    MILInterlockedAvailable
//
//  Synopsis:  Returns whether the processor is capable of performing
//             MILInterlockedCompareExchange64
//
//-----------------------------------------------------------------------------
__inline BOOL
MILInterlockedAvailable()
{
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:    MILInterlockedCompareExchange64
//
//  Synopsis:  Performs an atomic operation that does the following...
//             a) if (*pDestination == comperand)
//                   *pDestination = exchange
//             b) returns the value of *pDestination before the operation
//
//             Assembly implementation from base\ntos\ex\i386\intrlfst.asm
//-----------------------------------------------------------------------------
#pragma warning( push )
#pragma warning (disable: 4035)     // Disable function doesn't return value warning.
__inline __int64 
MILInterlockedCompareExchange64(
    __inout_ecount(1) __int64 volatile * pDestination, 
    __int64 exchange, 
    __int64 comperand
    )
{
    return reinterpret_cast<__int64>(InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID volatile *>(pDestination), 
        reinterpret_cast<PVOID>(exchange), 
        reinterpret_cast<PVOID>(comperand)
        ));
}
#pragma warning( pop )


#elif defined(_IA64_) || defined(_AMD64_) || defined(_AXP64_) 

//
// 64 bit Implementation- uses InterlockedCompareExchangePointer
//


//+----------------------------------------------------------------------------
//
//  Member:    MILInterlockedAvailable
//
//  Synopsis:  Returns whether the processor is capable of performing
//             MILInterlockedCompareExchange64
//
//-----------------------------------------------------------------------------
__inline BOOL
MILInterlockedAvailable()
{
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:    MILInterlockedCompareExchange64
//
//  Synopsis:  Performs an atomic operation that does the following...
//             a) if (*pDestination == comperand)
//                   *pDestination = exchange
//             b) returns the value of *pDestination before the operation
//
//-----------------------------------------------------------------------------
__inline __int64
MILInterlockedCompareExchange64(
    __inout_ecount(1) __int64 volatile * pllDestination,
    __int64 llExchange,
    __int64 llComperand
    )
{
    //
    // On 64-bit machines, an intrinsic 64-bit
    // InterlockedCompareExchangePointer  is already defined.
    //

    return reinterpret_cast<__int64>(InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID volatile *>(pllDestination), 
        reinterpret_cast<PVOID>(llExchange), 
        reinterpret_cast<PVOID>(llComperand)
        ));
}

#elif defined(_ARM_) || defined(_ARM64_)

//+----------------------------------------------------------------------------
//
//  Member:    MILInterlockedAvailable
//
//  Synopsis:  Returns whether the processor is capable of performing
//             MILInterlockedCompareExchange64
//
//-----------------------------------------------------------------------------
__inline BOOL
MILInterlockedAvailable()
{
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member:    MILInterlockedCompareExchange64
//
//  Synopsis:  Performs an atomic operation that does the following...
//             a) if (*pDestination == comperand)
//                   *pDestination = exchange
//             b) returns the value of *pDestination before the operation
//
//-----------------------------------------------------------------------------
__inline __int64
MILInterlockedCompareExchange64(
    __inout_ecount(1) __int64 volatile * pllDestination,
    __int64 llExchange,
    __int64 llComperand
    )
{
    return InterlockedCompareExchange64(pllDestination, llExchange, llComperand);
}

#else

#error Processor not recognized! No definition for MILInterlockedCompareExchange64 available
    
#endif


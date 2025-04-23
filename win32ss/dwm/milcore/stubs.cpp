/*
 * PROJECT:     ReactOS DWM Compatibility Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Stub file for dwmapi
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

//#define NDEBUG
extern "C"
{
    #include "milcore.h"

}


HRESULT WINAPI MilConnection_CreateChannel(
    HMIL_CONNECTION hConnection,
    MIL_CHANNEL hSourceChannel,
    MIL_CHANNEL *phChannel
    )
{
    return 0;
}
    
    
HRESULT WINAPI
MilChannel_GetMarshalType(
    MIL_CHANNEL hChannel,
    UINT32 *pMarshalType
    )
{
    return 0;
}

HRESULT WINAPI MilChannel_CommitChannel(
    MIL_CHANNEL hChannel
    )
    {
        return 0;
    }
#pragma once 

EXTERN_C
HRESULT WINAPI
MilResource_CreateOrAddRefOnChannel(
    MIL_CHANNEL hChannel,
    MIL_RESOURCE_TYPE type,
    __inout_ecount(1) HMIL_RESOURCE *ph
    );

EXTERN_C
HRESULT WINAPI
MilChannel_BeginCommand(
    MIL_CHANNEL hChannel,
    __in_bcount(cbCmd) VOID *pCmd,
    UINT32 cbCmd,
    UINT32 cbExtra
    );

EXTERN_C 
HRESULT WINAPI
MilChannel_AppendCommandData(
    MIL_CHANNEL hChannel,
    __in_bcount(cbSize) VOID *pvData,
    UINT32 cbSize
    );

EXTERN_C
HRESULT WINAPI
MilChannel_EndCommand(
    MIL_CHANNEL hChannel
    );

EXTERN_C
HRESULT WINAPI
MilResource_SendCommand(
    __in_bcount(cbSize) VOID *pvCommandData,
    UINT32 cbSize,
   // bool sendInSeparateBatch, Not In Vista RTM?
    MIL_CHANNEL hChannel
    );

EXTERN_C
HRESULT WINAPI
MilResource_ReleaseOnChannel(
    MIL_CHANNEL hChannel,
    HMIL_RESOURCE h,
    __out_ecount_opt(1) BOOL *pfDeleted
    );

EXTERN_C
HRESULT WINAPI MilChannel_CommitChannel(
    MIL_CHANNEL hChannel
    );

EXTERN_C
HRESULT WINAPI MilConnection_CreateChannel(
    HMIL_CONNECTION hConnection,
    MIL_CHANNEL hSourceChannel,
    MIL_CHANNEL *phChannel
    );

EXTERN_C
HRESULT WINAPI
MilChannel_GetMarshalType(
    MIL_CHANNEL hChannel,
    __out_ecount(1) MilMarshalType::Enum *pMarshalType
    );

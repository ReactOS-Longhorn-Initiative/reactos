#pragma once 

EXTERN_C
HRESULT WINAPI
MilResource_CreateOrAddRefOnChannel(
    MIL_CHANNEL hChannel,
    MIL_RESOURCE_TYPE type,
    __inout_ecount(1) HMIL_RESOURCE *ph
    );
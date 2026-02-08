#pragma once

class MilResource
{
private:

public:
    HRESULT WINAPI InitializeResource(MIL_RESOURCE_TYPE type, MIL_CHANNEL MilChannel);
    HRESULT WINAPI SendCommand(PVOID Command, UINT32 Size);
    HRESULT WINAPI SendCommandWithData(PVOID Command, UINT32 Size, PVOID ExtraData, UINT32 ExtraDataSize);
    HRESULT WINAPI EndCommand(VOID);
    HRESULT WINAPI AppendCommandData(PVOID ExtraData, UINT32 ExtraDataSize);
    MilResource();
    ~MilResource();
    HMIL_RESOURCE GlobalResourceHandle;
    MIL_CHANNEL GlobalMilChannel;
};

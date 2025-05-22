#include <uDWM.h>
#include <debug.h>

HRESULT
WINAPI
MilResourceAdaptType(MIL_CHANNEL MilChannel,
                     HMIL_RESOURCE ResourceHandle,
                     MilResource **MilResourceInstance)
{
    HRESULT hr = S_OK;
    MilResource* MilResourceClass;
    MilResourceClass = new MilResource();
    if (MilResourceClass == NULL)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }
   
    MilResource_CreateOrAddRefOnChannel(MilChannel, TYPE_NULL, &ResourceHandle);
    MilResourceClass->GlobalResourceHandle = ResourceHandle;
    MilResourceClass->GlobalMilChannel = MilChannel;
    *MilResourceInstance = MilResourceClass;
    return hr;
}

MilResource::MilResource()
{

}

MilResource::~MilResource()
{
    MilResource_ReleaseOnChannel(GlobalMilChannel, GlobalResourceHandle, NULL);
    GlobalResourceHandle = NULL;
    GlobalMilChannel = NULL;
}

HRESULT
WINAPI
MilResource::InitializeResource(MIL_RESOURCE_TYPE type, MIL_CHANNEL MilChannel)
{
    GlobalMilChannel = MilChannel;
    HRESULT hr = MilResource_CreateOrAddRefOnChannel(MilChannel, type, &GlobalResourceHandle);
    if (FAILED(hr))
    {
        DPRINT1("Failed to create or add reference on channel\n");
        return hr;
    }

    return hr;
}

HRESULT
WINAPI
MilResource::AppendCommandData(PVOID ExtraData, UINT32 ExtraDataSize)
{
    HRESULT hr = S_OK;
    hr = MilChannel_AppendCommandData(GlobalMilChannel, ExtraData, ExtraDataSize);
    if (FAILED(hr))
    {
        DPRINT1("Failed to append command data on channel\n");
        return hr;
    }

    return hr;
}

HRESULT
WINAPI
MilResource::SendCommand(PVOID Command, UINT32 Size)
{
    HRESULT hr = S_OK;
    hr = MilResource_SendCommand(Command, Size, GlobalMilChannel);

    return hr;
}

HRESULT
WINAPI
MilResource::EndCommand(VOID)
{
    HRESULT hr = S_OK;
    hr = MilChannel_EndCommand(GlobalMilChannel);
    if (FAILED(hr))
    {
        DPRINT1("Failed to end command on channel\n");
        return hr;
    }

    return hr;
}

HRESULT
WINAPI
MilResource::SendCommandWithData(PVOID Command, UINT32 Size, PVOID ExtraData, UINT32 ExtraDataSize)
{
    HRESULT hr = S_OK;
    hr = MilChannel_BeginCommand(GlobalMilChannel, Command, Size, ExtraDataSize);
    if (FAILED(hr))
    {
        DPRINT1("Failed to send command to channel\n");
        return hr;
    }
    else
    {
        MilChannel_AppendCommandData(GlobalMilChannel, ExtraData, ExtraDataSize);
        MilChannel_EndCommand(GlobalMilChannel);
    }

    return hr;
}

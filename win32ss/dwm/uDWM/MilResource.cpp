#include <uDWM.h>
#include <debug.h>
/* 
 * For abstraction purses, every resource in uDWM should be based on this MilResource Class.
 */
HRESULT
WINAPI
MilResourceCreateType(MIL_RESOURCE_TYPE type,
                      MIL_CHANNEL MilChannel,
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

    MilResourceClass->InitializeResource(type, MilChannel);

    return hr;
}

MilResource::MilResource()
{

}

MilResource::~MilResource()
{

}

HRESULT
WINAPI
MilResource::InitializeResource(MIL_RESOURCE_TYPE type, MIL_CHANNEL MilChannel)
{
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
MilResource::SendCommand(PVOID Command, UINT32 Size)
{
    return 0;
}

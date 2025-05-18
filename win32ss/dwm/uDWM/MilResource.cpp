#include <uDWM.h>

/* 
 * For abstraction purses, every resource in uDWM should be based on this MilResource Class.
 */
HRESULT
WINAPI
MilResourceCreateType(MIL_RESOURCE_TYPE type,
                      HMIL_CHANNEL MilChannel,
                      MilResource **MilResourceInstance)
{
    HRESULT hr = S_OK;

    *MilResourceInstance = new MilResource();
    if (*MilResourceInstance == NULL)
    {
        hr = E_OUTOFMEMORY;
        return hr;
    }

    return hr;
}

MilResource::MilResource()
{

}

MilResource::~MilResource()
{

}

#include "uDWM.h"

#include <debug.h>
#include "RwmVistaSp1MilCmd.h"

/*
 * For abstraction purses, every resource in uDWM should be based on this MilResource Class.
 */
EXTERN_C
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
    *MilResourceInstance = MilResourceClass;
    return hr;
}

DwmVisual::DwmVisual()
{

}

DwmVisual::~DwmVisual()
{

}


HRESULT
WINAPI
DwmVisual::DrawBullshit()
{
#if UDWM_TARGET_VISTA_SP1_MILCORE
    /*
     * Disabled for Vista SP1 interop.
     *
     * Our tree is WindowNode-based; emitting "value resource" commands here is
     * unrelated to the desktop composition path and historically poisoned the
     * command stream when tested against Vista's milcore.dll.
     */
    return E_NOTIMPL;
#else
    MILCMD_COLORRESOURCE cmd = {};
    cmd.Type =
        MilCmdColorResource;
    cmd.Handle = MilResource->GlobalResourceHandle;
    cmd.Value = {1.0f, 1.0f, 1.0f, 1.0f};
    MilResource->SendCommand(
        (PVOID)&cmd,
        sizeof(MILCMD_COLORRESOURCE));
        return S_OK;
#endif
}

EXTERN_C
HRESULT
WINAPI
MilResourceCreateType(MIL_RESOURCE_TYPE type,
                      MIL_CHANNEL MilChannel,
                      MilResource **MilResourceInstance);
VOID
DwmVisual::HideVisual()
{
    MILCMD_VISUAL_SETALPHA cmd = {};
    cmd.alpha = 0.0;
    cmd.Type =
#if UDWM_TARGET_VISTA_SP1_MILCORE
        (MILCMD)RWM_MILCMD_VSP1_VISUAL_SETALPHA;
#else
        MilCmdVisualSetAlpha;
#endif
    MilResource->SendCommand(
        (PVOID)&cmd,
        sizeof(MILCMD_VISUAL_SETALPHA));
}

HRESULT
DwmVisual::Initialize(MIL_CHANNEL hChannel)
{
    HRESULT hr = S_OK;
    GlobalChannel = hChannel;
    hr = MilResourceCreateType(TYPE_VISUAL,
                              hChannel,
                              &MilResource);
    if (FAILED(hr))
    {
        DPRINT1("Failed to create visual resource\n");
        return hr;
    }

    return S_OK;
}


HRESULT
WINAPI
CreateDwmVisual(MIL_CHANNEL const hChannel, DwmVisual **DwmVisualReturn)
{
    *DwmVisualReturn = new DwmVisual();
    if (*DwmVisualReturn == NULL)
    {
        return E_OUTOFMEMORY;
    }
    return (*DwmVisualReturn)->Initialize(hChannel);
}

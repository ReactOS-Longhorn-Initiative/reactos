#pragma once

#include "MilResource.hpp"

class DwmVisual
{
private:
    MilResource *MilResource;
    MIL_CHANNEL GlobalChannel;
public:
    DwmVisual();
    ~DwmVisual();
    VOID    HideVisual();
    HMIL_RESOURCE GetHandle() const { return MilResource ? MilResource->GlobalResourceHandle : 0; }
HRESULT
WINAPI
 DrawBullshit();
    HRESULT Initialize(MIL_CHANNEL hChannel);
};


HRESULT
WINAPI
CreateDwmVisual(MIL_CHANNEL const hChannel, DwmVisual **DwmVisualReturn);

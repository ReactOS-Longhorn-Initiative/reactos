#pragma once

class DwmVisual
{
private:
    MilResource *MilResource;
    MIL_CHANNEL GlobalChannel;
public:
    DwmVisual();
    ~DwmVisual();
    VOID    HideVisual();
HRESULT
WINAPI
 DrawBullshit();
    HRESULT Initialize(MIL_CHANNEL hChannel);
};


HRESULT
WINAPI
CreateDwmVisual(MIL_CHANNEL const hChannel, DwmVisual **DwmVisualReturn);

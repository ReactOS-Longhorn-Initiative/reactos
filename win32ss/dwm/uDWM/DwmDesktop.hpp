#pragma once

class DwmDesktop
{
private:
UINT32 MarshalType;
public:
MIL_CHANNEL GlobalChannel;
CRITICAL_SECTION CsDwmInstance;
DwmDesktop();
~DwmDesktop();
HRESULT Initialize(PRWM_STARTUPINFO StartupInfo, PRWM_COMPOSITIONINFO CompInfo);
};

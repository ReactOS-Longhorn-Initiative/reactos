#pragma once

class DwmDesktop
{
private:
UINT32 MarshalType;
public:
MIL_CHANNEL GlobalChannel;
CRITICAL_SECTION CsDwmInstance;
HMIL_RESOURCE hDesktopTarget;
HMIL_RESOURCE hRootNode;
BOOLEAN RootIsDesktopClone;
DwmDesktop();
~DwmDesktop();
HRESULT EnsureDesktopTargetAndRoot();
HRESULT Initialize(PRWM_STARTUPINFO StartupInfo, PRWM_COMPOSITIONINFO CompInfo);
};

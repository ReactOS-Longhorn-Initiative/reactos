#pragma once
extern "C"
{
#include "uDWM.h"
}
#include "classes/CWindowList.hpp"
#include <d3d9types.h>

class CDesktopManager
{
private:
MIL_CHANNEL m_hChannel;
CRITICAL_SECTION s_csDwmInstance;
public:
CDesktopManager();
~CDesktopManager();
HRESULT   LoadTheme();
VOID      SetupColorization(BOOLEAN A1, BOOLEAN A2);
VOID      GetBlendedColorizationColor(BOOLEAN a1, BOOLEAN a2, float a3, D3DCOLORVALUE *ColorValue);
HRESULT   SetColorizationColorResource(struct CResource *a1, bool a2, bool a3, float a4);
VOID      UnloadTheme();
HRESULT   Initialize(StartupInfo *StartupInfo, CompositionInfo *CompInfo);
DWORD WINAPI uDWMWndProc(LPVOID lpThreadParameter);
};
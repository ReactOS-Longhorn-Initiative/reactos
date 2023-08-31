
#include "slwga.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                    DWORD fdwReason,
                    LPVOID fImpLoad)

{
    /* For now, there isn't much to do */
    if (fdwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hinstDLL);
    return TRUE;
}


typedef GUID SLID;


typedef enum _SL_GENUINE_STATE {
  SL_GEN_STATE_IS_GENUINE = 0,
  SL_GEN_STATE_INVALID_LICENSE,
  SL_GEN_STATE_TAMPERED,
  SL_GEN_STATE_OFFLINE,
  SL_GEN_STATE_LAST
} SL_GENUINE_STATE;

typedef struct _tagSL_NONGENUINE_UI_OPTIONS {
  DWORD      cbSize;
  const SLID *pComponentId;
  HRESULT    hResultUI;
} SL_NONGENUINE_UI_OPTIONS;

HRESULT
WINAPI
SLIsGenuineLocal(_In_    SLID               *pAppId,
                 _Out_   SL_GENUINE_STATE         *pGenuineState,
                 _Inout_ SL_NONGENUINE_UI_OPTIONS *pUIOptions)
{
    return S_OK;
}

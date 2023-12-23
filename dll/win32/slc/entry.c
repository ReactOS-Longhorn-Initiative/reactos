
#include "slc.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                    DWORD fdwReason,
                    LPVOID fImpLoad)

{
    /* For now, there isn't much to do */
    if (fdwReason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hinstDLL);
    return TRUE;
}

typedef enum _tagSLDATATYPE {
  SL_DATA_NONE = REG_NONE,
  SL_DATA_SZ = REG_SZ,
  SL_DATA_DWORD = REG_DWORD,
  SL_DATA_BINARY = REG_BINARY,
  SL_DATA_MULTI_SZ,
  SL_DATA_SUM = 100
} SLDATATYPE;


HRESULT WINAPI SLGetWindowsInformation(
  PCWSTR     pwszValueName,
  SLDATATYPE *peDataType,
  UINT       *pcbValue,
  PBYTE      *ppbValue
)
{
    return S_OK;
}
extern "C"
{
    #include "uDWM.h"
}
#include "CDesktopManager.hpp"

#include <debug.h>

CDesktopManager* Global_DesktopManageInst;

EXTERN_C
HRESULT
CDesktopManager_Create(StartupInfo *StartupInfo, CompositionInfo *CompInfo)
{
    HRESULT Status;
    DPRINT1("CDesktopManager_Create: Entry\n");

    Global_DesktopManageInst = new CDesktopManager();
    if (!Global_DesktopManageInst)
    {
        Status = -1;
        DPRINT1("Couldn't allocate Global_DesktopManageInst\n");
        return Status;
    }

    Status = Global_DesktopManageInst->Initialize(StartupInfo, CompInfo);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("Global_DesktopManageInst: Could not Initialize uDWM\n");
    }

    return Status;
}

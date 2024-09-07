 
#include "uxthemep.h"


PVOID //TODO:
WINAPI
Serv_SessionAlloc(PVOID hProcess,
                 UINT32 dwServerChangeNumber,
                 PVOID PfnRegister,
                 PVOID PfnClearStockObjects,
                 PVOID PfnLoadTheme,
                 UINT32 StackSizedReserved,
                 UINT32 StackSizeCommit)
{
    UNIMPLEMENTED;
    return HeapAlloc(GetProcessHeap(), 0, 0x100);
}
#include "dwr.hpp"
#include <debug.h>

/* C++ Entry point */
int WINAPI
DwmEntry(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nShowCmd)
{
    UINT32 ExitCode;
    DWRUserFace AppHostInstance;

    RWMCreateSessionPort();
    RWMConnectToUxServ();
    /* Now let's create a window */
    ExitCode = AppHostInstance.Initialize(hInstance);
    DPRINT1("CDwmAppHost::Initialize -> Exit code: %d\n", ExitCode);
    AppHostInstance.Run();
    AppHostInstance.Cleanup();
    return 0;
}

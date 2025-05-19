#include "rwm.hpp"
#include <debug.h>

/* C++ Entry point */
int WINAPI
DwmEntry(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nShowCmd)
{
    UINT32 ExitCode;
    RWMUserFace AppHostInstance;

    /* Now let's create a window */
    ExitCode = AppHostInstance.Initialize(hInstance);
    DPRINT1("CDwmAppHost::Initialize -> Exit code: %d\n", ExitCode);
    RWMCreateSessionPort();
    RWMConnectToUxServ();
    AppHostInstance.Run();
    AppHostInstance.Cleanup();
    return 0;
}

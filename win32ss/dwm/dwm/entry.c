#include "dwm.h"

int WINAPI
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nShowCmd)
{
    return DwmEntry(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

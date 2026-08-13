/* themestub.dll - an empty resource-only DLL used as the carrier that the
   theme compiler injects packed visual-style resources into via UpdateResource. */
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    (void)hinst;
    (void)reason;
    (void)reserved;
    return TRUE;
}

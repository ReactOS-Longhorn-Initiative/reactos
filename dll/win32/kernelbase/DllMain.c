
#include <windows.h>

void init_locale( HMODULE module );

/* Wine defines this in kernelbase's main.c, which we cannot build yet (it
 * needs init_locale(), still inside locale.c's #ifndef __REACTOS__ block).
 * file.c and process.c read it, so provide it here until main.c comes up. */
BOOL is_wow64 = FALSE;

/* system_dir used to be defined here as a char[] stand-in for locale.c. Now
 * that file.c builds it supplies the real "const WCHAR system_dir[]" that
 * kernelbase.h declares, and locale.c's only uses of it are still inside the
 * #ifndef __REACTOS__ block, so the stand-in would just collide. */

static
void
InitIsWow64(void)
{
    if (!IsWow64Process(GetCurrentProcess(), &is_wow64))
        is_wow64 = FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            InitIsWow64();
            //init_locale(hinstDLL);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

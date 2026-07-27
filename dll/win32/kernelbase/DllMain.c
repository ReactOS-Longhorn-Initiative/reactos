
#include <windows.h>

void init_locale( HMODULE module );

// Used by wine/locale.c
char system_dir[MAX_PATH];

/* Wine defines this in kernelbase's main.c, which we cannot build yet (it
 * needs init_locale(), still inside locale.c's #ifndef __REACTOS__ block).
 * file.c and process.c read it, so provide it here until main.c comes up. */
BOOL is_wow64 = FALSE;

static
void
InitSystemDir(void)
{
    GetSystemDirectoryA(system_dir, MAX_PATH);
}

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
            InitSystemDir();
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

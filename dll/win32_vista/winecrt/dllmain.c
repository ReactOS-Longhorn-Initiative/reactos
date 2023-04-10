
#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(winecrt);

/* EXTERNAL PROTOTYPES ********************************************************/

BOOL crt_process_init(void);

extern void FreeEnvironment(char **environment);

#undef _environ
extern char** _environ;      /* pointer to environment block */
extern char** __initenv;     /* pointer to initial environment block */
extern wchar_t** _wenviron;  /* pointer to environment block */
extern wchar_t** __winitenv; /* pointer to initial environment block */

/* LIBRARY ENTRY POINT ********************************************************/

BOOL
WINAPI
DllMain(PVOID hinstDll, ULONG dwReason, PVOID reserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        TRACE("Process Attach\n");
        TRACE("Attach done\n");
        break;

    case DLL_THREAD_ATTACH:
        //winecrt_get_thread_data creates data when first called
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

/* EOF */

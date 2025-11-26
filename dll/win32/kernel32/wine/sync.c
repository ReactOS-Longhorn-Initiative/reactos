#include <k32.h>

#define NDEBUG
#include <debug.h>

/***********************************************************************
 *           CreateWaitableTimerExA    (KERNEL32.@)
 */
HANDLE WINAPI CreateWaitableTimerExA( SECURITY_ATTRIBUTES *sa, LPCSTR name, DWORD flags, DWORD access )
{
    WCHAR buffer[MAX_PATH];

    if (!name) return CreateWaitableTimerExW( sa, NULL, flags, access );

    if (!MultiByteToWideChar( CP_ACP, 0, name, -1, buffer, MAX_PATH ))
    {
        SetLastError( ERROR_FILENAME_EXCED_RANGE );
        return 0;
    }
    return CreateWaitableTimerExW( sa, buffer, flags, access );
}

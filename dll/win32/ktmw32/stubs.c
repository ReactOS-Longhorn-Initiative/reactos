#include "KB.h"
#include <debug.h>

HANDLE WINAPI CreateTransaction( SECURITY_ATTRIBUTES *sa, GUID *guid, DWORD options, DWORD level, DWORD flags,
        DWORD timeout, WCHAR *desc )
{
    ULONG obj_flags = OBJ_CASE_INSENSITIVE;
    UNICODE_STRING desc_str;
    OBJECT_ATTRIBUTES attr;
    //LARGE_INTEGER time;
   // HANDLE handle;

    if (sa && sa->bInheritHandle) obj_flags |= OBJ_INHERIT;
    InitializeObjectAttributes( &attr, NULL, obj_flags, 0, sa ? sa->lpSecurityDescriptor : NULL );

    RtlInitUnicodeString( &desc_str, desc );
#if 0
     oh god this is a vista+ syscall, yeah that aint happening
    NtCreateTransaction( &handle, 0 /* FIXME */, &attr, guid, NULL, options, level, flags,
            100, &desc_str );
#endif
    return NULL;
}

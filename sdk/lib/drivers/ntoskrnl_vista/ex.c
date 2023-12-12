
#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>

NTKRNLVISTAAPI
PEX_TIMER
NTAPI
ExAllocateTimer(_In_opt_ PEXT_CALLBACK Callback,
                _In_opt_ PVOID CallbackContext,
                _In_ ULONG Attributes)
{
    UNIMPLEMENTED;
	return NULL;
}

NTKRNLVISTAAPI
BOOLEAN
NTAPI
ExCancelTimer(_Inout_ PEX_TIMER Timer,
              _In_opt_ PEXT_CANCEL_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
	return TRUE;
}

NTKRNLVISTAAPI
BOOLEAN
NTAPI
ExDeleteTimer(_In_ PEX_TIMER Timer,
              _In_ BOOLEAN Cancel,
              _In_ BOOLEAN Wait,
              _In_ PEXT_DELETE_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
	return Cancel;
}

NTKRNLVISTAAPI
BOOLEAN
NTAPI
ExSetTimer(_In_ PEX_TIMER Timer,
           _In_ LONGLONG DueTime,
           _In_ LONGLONG Period,
           _In_opt_ PEXT_SET_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
	return FALSE;
}


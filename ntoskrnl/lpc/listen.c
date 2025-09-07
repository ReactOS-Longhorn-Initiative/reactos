
/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtListenPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE ConnectionRequest)
{
    PAGED_CODE();
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

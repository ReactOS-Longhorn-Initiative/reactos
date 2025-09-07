

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE LpcPortObjectType, LpcWaitablePortObjectType;

/* PRIVATE FUNCTIONS *********************************************************/


/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtImpersonateClientOfPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE ClientMessage)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtQueryPortInformationProcess(VOID)
{
    /* This is all this function does */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
NtQueryInformationPort(
    _In_ HANDLE PortHandle,
    _In_ PORT_INFORMATION_CLASS PortInformationClass,
    _Out_bytecap_(PortInformationLength) PVOID PortInformation,
    _In_ ULONG PortInformationLength,
    _Out_ PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

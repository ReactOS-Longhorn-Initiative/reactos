


/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtReplyPort(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE LpcReply)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtReplyWaitReplyPort(
    _In_ HANDLE PortHandle,
    _Out_ PPORT_MESSAGE ReplyMessage)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage,
    _In_opt_ PLARGE_INTEGER Timeout)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtReplyWaitReceivePort(
    _In_ HANDLE PortHandle,
    _Out_opt_ PVOID *PortContext,
    _In_opt_ PPORT_MESSAGE ReplyMessage,
    _Out_ PPORT_MESSAGE ReceiveMessage)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReadRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG Index,
    _Out_bytecap_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtWriteRequestData(
    _In_ HANDLE PortHandle,
    _In_ PPORT_MESSAGE Message,
    _In_ ULONG Index,
    _In_bytecount_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLengthh)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

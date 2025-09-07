
/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/* PUBLIC FUNCTIONS **********************************************************/


// Internal helper to convert legacy LPC params to ALPC_PORT_ATTRIBUTES
static VOID LpcLegacyToAlpcPortAttributes(
    PALPC_PORT_ATTRIBUTES AlpcAttr,
    ULONG MaxConnectionInfoLength,
    ULONG MaxMessageLength,
    ULONG MaxPoolUsage)
{
    RtlZeroMemory(AlpcAttr, sizeof(*AlpcAttr));
    AlpcAttr->MaxMessageLength = MaxMessageLength;
    AlpcAttr->MaxPoolUsage = MaxPoolUsage;
    // Set other fields as needed
}

NTSTATUS
NTAPI
NtCreatePort(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectionInfoLength,
    _In_ ULONG MaxMessageLength,
    _In_ ULONG MaxPoolUsage)
{
    PAGED_CODE();

    ALPC_PORT_ATTRIBUTES AlpcAttr;
    LpcLegacyToAlpcPortAttributes(&AlpcAttr, MaxConnectionInfoLength, MaxMessageLength, MaxPoolUsage);
    return AlpcCreatePort(PortHandle, ObjectAttributes, &AlpcAttr);
}

NTSTATUS
NTAPI
NtCreateWaitablePort(
    _Out_ PHANDLE PortHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG MaxConnectInfoLength,
    _In_ ULONG MaxDataLength,
    _In_opt_ ULONG NPMessageQueueSize)
{
    PAGED_CODE();

    ALPC_PORT_ATTRIBUTES AlpcAttr;
    LpcLegacyToAlpcPortAttributes(&AlpcAttr, MaxConnectInfoLength, MaxDataLength, NPMessageQueueSize ? NPMessageQueueSize : 0);
    AlpcAttr.Flags |= 0x1; // Example: set waitable flag (define properly later)
    return AlpcCreatePort(PortHandle, ObjectAttributes, &AlpcAttr);
}

/* EOF */

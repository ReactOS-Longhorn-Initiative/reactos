

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "alpc.h"
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/* PUBLIC FUNCTIONS **********************************************************/


// Internal helper to convert legacy PORT_VIEW/REMOTE_PORT_VIEW to ALPC_PORT_VIEW/ALPC_REMOTE_PORT_VIEW
static VOID LpcLegacyToAlpcViews(
    PALPC_PORT_VIEW AlpcView,
    PALPC_REMOTE_PORT_VIEW AlpcRemoteView,
    PPORT_VIEW ClientView,
    PREMOTE_PORT_VIEW ServerView)
{
    if (AlpcView && ClientView)
    {
        RtlZeroMemory(AlpcView, sizeof(*AlpcView));
        AlpcView->Length = sizeof(*AlpcView);
        AlpcView->SectionHandle = ClientView->SectionHandle;
        AlpcView->SectionOffset = ClientView->SectionOffset;
        AlpcView->ViewSize = ClientView->ViewSize;
        AlpcView->ViewBase = ClientView->ViewBase;
        AlpcView->ViewRemoteBase = NULL; // Not present in legacy
    }
    if (AlpcRemoteView && ServerView)
    {
        RtlZeroMemory(AlpcRemoteView, sizeof(*AlpcRemoteView));
        AlpcRemoteView->Length = sizeof(*AlpcRemoteView);
        AlpcRemoteView->ViewSize = ServerView->ViewSize;
        AlpcRemoteView->ViewBase = ServerView->ViewBase;
    }
}

NTSTATUS
NTAPI
NtConnectPort(
    _Out_ PHANDLE PortHandle,
    _In_ PUNICODE_STRING PortName,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    _Inout_opt_ PPORT_VIEW ClientView,
    _Inout_opt_ PREMOTE_PORT_VIEW ServerView,
    _Out_opt_ PULONG MaxMessageLength,
    _Inout_opt_ PVOID ConnectionInformation,
    _Inout_opt_ PULONG ConnectionInformationLength)
{
    ALPC_PORT_ATTRIBUTES AlpcAttr;
    RtlZeroMemory(&AlpcAttr, sizeof(AlpcAttr));
    if (SecurityQos)
    {
        AlpcAttr.SecurityQos = *SecurityQos;
        AlpcAttr.SecurityQos.Length = sizeof(*SecurityQos);
    }
    ALPC_PORT_VIEW AlpcView;
    ALPC_REMOTE_PORT_VIEW AlpcRemoteView;
    LpcLegacyToAlpcViews(&AlpcView, &AlpcRemoteView, ClientView, ServerView);
    return AlpcConnectPort(PortHandle, PortName, &AlpcAttr, ClientView ? &AlpcView : NULL, ServerView ? &AlpcRemoteView : NULL, ConnectionInformation, ConnectionInformationLength);
}

NTSTATUS
NTAPI
NtCompleteConnectPort(_In_ HANDLE PortHandle)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

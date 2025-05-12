
/* INCLUDES *****************************************************************/

#include "uxsms.h"

#include <debug.h>

#define DWMUXSMS_APIPORTDESCRIPTION L"User Experience SubSystem API Port"

/* GLOBALS ******************************************************************/

//static WCHAR PortName[] = L"\\UxSmsApiPort";

/* FUNCTIONS ****************************************************************/

VOID
WINAPI
InitializeServicePort()
{
    NTSTATUS Status;
    HANDLE PortHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PortString;

    RtlInitUnicodeString(&PortString, PortName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &PortString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreatePort(&PortHandle,
                          &ObjectAttributes,
                          (sizeof(L"User Experience Session Management Service API Port")),
                          256,
                          16 * 256);

    if (!NT_SUCCESS(Status))
        DbgPrint("Failed to create port: %lx\n", Status);
}


/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/sysinfo.c
 * PURPOSE:         HAL Information Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
HaliQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
                           IN ULONG BufferSize,
                           IN OUT PVOID Buffer,
                           OUT PULONG ReturnedLength)
{
	UNIMPLEMENTED;
    while (TRUE);
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HaliSetSystemInformation(IN HAL_SET_INFORMATION_CLASS InformationClass,
                         IN ULONG BufferSize,
                         IN OUT PVOID Buffer)
{
    if (InformationClass == HalProcessorSpeedInformation)
    {
        PHAL_PROCESSOR_SPEED_INFORMATION SpeedInfo = Buffer;

        if (Buffer == NULL || BufferSize < sizeof(HAL_PROCESSOR_SPEED_INFORMATION))
            return STATUS_INFO_LENGTH_MISMATCH;

        if (SpeedInfo->ProcessorSpeed > 100)
            SpeedInfo->ProcessorSpeed = 100;

        return STATUS_SUCCESS;
    }

    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */

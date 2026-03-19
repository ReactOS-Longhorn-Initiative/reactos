/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power Manager thermal request implementation support
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/**
 * @brief
 * The thermal request kernel object type, created once at initialization
 * time by PopCreateThermalRequestObjectType and used by PoCreateThermalRequest
 * to allocate thermal request (cooling extension) objects.
 */
POBJECT_TYPE PoThermalRequestObjectType = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * The close procedure for a thermal request (cooling extension) kernel object.
 * Invoked by the Object Manager when the last handle to a thermal request object
 * is closed. This routine disables the cooling extension, unlinks it from any
 * DOPE (Device Object Power Extension) it is associated with, and frees any
 * PnP notification entry it may hold.
 *
 * @param[in] Process
 * A pointer to the process that is closing the handle. Optional.
 *
 * @param[in] ThermalRequestObject
 * A pointer to the thermal request (cooling extension) object to close.
 *
 * @param[in] GrantedAccess
 * The access rights that were granted to the handle at the time it was opened.
 *
 * @param[in] ProcessHandleCount
 * The number of handles to this object that the process currently holds.
 *
 * @param[in] SystemHandleCount
 * The total number of handles to this object across all processes.
 */
VOID
NTAPI
PopCloseThermalRequestObject(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID ThermalRequestObject,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG ProcessHandleCount,
    _In_ ULONG SystemHandleCount)
{
    PPOP_COOLING_EXTENSION CoolingExtension;

    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(GrantedAccess);
    UNREFERENCED_PARAMETER(ProcessHandleCount);

    /*
     * Only perform cleanup when the last system-wide handle to this object
     * is being closed (SystemHandleCount goes to 0).
     */
    if (SystemHandleCount != 0)
    {
        return;
    }

    CoolingExtension = (PPOP_COOLING_EXTENSION)ThermalRequestObject;

    /* Mark the cooling extension as disabled so no further operations apply */
    CoolingExtension->Enabled = FALSE;

    /*
     * If this cooling extension is associated with a PnP notification entry,
     * deregister it now so the Power Manager no longer receives device removal
     * notifications for the target device being cooled.
     */
    if (CoolingExtension->NotificationEntry != NULL)
    {
        IoUnregisterPlugPlayNotification(CoolingExtension->NotificationEntry);
        CoolingExtension->NotificationEntry = NULL;
    }

    /*
     * Dereference the calling interface to decrement its active reference count,
     * signaling to the provider that this cooling request is going away.
     */
    if (CoolingExtension->Interface.InterfaceDereference != NULL)
    {
        CoolingExtension->Interface.InterfaceDereference(CoolingExtension->Interface.Context);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/* EOF */

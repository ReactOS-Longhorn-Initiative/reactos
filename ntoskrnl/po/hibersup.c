/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Power system hibernation infrastructure support
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Marks the specific area of RAM as the snapshot for hibernation.
 * The Power Manager will take that range into consideration when
 * the system undergoes hibernation as it'll clone the whole specific
 * area of RAM into the hibernation file.
 *
 * @param[in] HiberContext
 * A pointer to arbitrary data. This usually points to the hibernation
 * context of the system which keeps track of all hibernation shenanigans.
 *
 * @param[in] Flags
 * A flag bit passed by the caller of which influences how this function
 * behaves. The following flags are:
 *
 * PO_MEM_PRESERVE -- The following memory range needs to be preserved.
 *                    The Power Manager is held responsible to preserve
 *                    these pages for the whole life time.
 *
 * PO_MEM_CLONE -- Keep a copy of the memory pages specified by the start
 *                 of the range.
 *
 * PO_MEM_CL_OR_NCHK -- Similar to the flag above, except that if cloning
 *                      is not possible for whatever reason, do not perform
 *                      checksum integrity checks against the target pages.
 *
 * PO_MEM_DISCARD -- Tells the Power Manager the memory pages in RAM starting
 *                   with the specific range are not to be considered for
 *                   conservation in the hibernation file. As such, all that
 *                   memory will be discarded during a low-power transition.
 *
 * PO_MEM_PAGE_ADDRESS -- Tells the Power Manager that the starting range
 *                        are physical pages. Physical memory pages are handled
 *                        differently from the virtual ones.
 *
 * PO_MEM_BOOT_PHASE -- Reserved by the system. This indicates that the memory
 *                      pages are to be handled differently during the boot phase.
 *
 * @param[in] StartPage
 * A pointer to arbitrary data. This usually points to the starting range of RAM
 * of which this function marks the hibernation range.
 *
 * @param[in] Length
 * The length of the memory range in RAM to be marked for hibernation,
 * provided by the caller.
 *
 * @param[in] PageTag
 * The tag that identifies the page range, provided by the caller.
 */
VOID
NTAPI
PoSetHiberRange(
    _In_ PVOID HiberContext,
    _In_ ULONG Flags,
    _In_ PVOID StartPage,
    _In_ ULONG Length,
    _In_ ULONG PageTag)
{
    PPOP_HIBER_CONTEXT Context;

    /*
     * If the caller passed a NULL hibernation context this means the
     * caller wants the global Power Manager action context to be used
     * instead.
     */
    if (!HiberContext)
    {
        Context = PopAction.HiberContext;

        /*
         * The global action context has no hibernation context if the system
         * is not undergoing hibernation at the moment.
         */
        if (!Context)
        {
            return;
        }
    }
    else
    {
        Context = (PPOP_HIBER_CONTEXT)HiberContext;
    }

    /*
     * If the hibernation context has already failed, do not attempt
     * to modify it any further.
     */
    if (!NT_SUCCESS(Context->Status))
    {
        return;
    }

    /*
     * Hibernation range marking is not yet implemented in ReactOS.
     * Full hibernation file I/O, page collection, and range map management
     * require the complete memory manager hibernation infrastructure.
     * This function serves as the registration point that HAL and drivers
     * use to tell the Power Manager which memory ranges must be included
     * in the hibernation image.
     *
     * When the hibernation infrastructure is complete, this function will:
     *   1. Look up the range map within the hibernation context
     *   2. Clamp the length to page boundaries
     *   3. Depending on the Flags, mark pages as preserved (PO_MEM_PRESERVE),
     *      cloned (PO_MEM_CLONE / PO_MEM_CL_OR_NCHK), discarded (PO_MEM_DISCARD),
     *      or boot-phase special (PO_MEM_BOOT_PHASE). Physical-page addresses
     *      (PO_MEM_PAGE_ADDRESS) are handled separately from virtual ones.
     *   4. Tag the range with PageTag for diagnostic/debugging purposes.
     */
    DPRINT("PoSetHiberRange: Context 0x%p, Flags 0x%lx, StartPage 0x%p, Length 0x%lx, Tag 0x%lx\n",
           Context, Flags, StartPage, Length, PageTag);
}

/* EOF */

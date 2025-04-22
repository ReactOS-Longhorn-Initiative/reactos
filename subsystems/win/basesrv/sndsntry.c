/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/sndsntry.c
 * PURPOSE:         Sound Sentry Notifications
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "basesrv.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef BOOL (NTAPI *PUSER_SOUND_SENTRY)(VOID);
BOOL NTAPI FirstSoundSentry(VOID);

static PUSER_SOUND_SENTRY _UserSoundSentry = FirstSoundSentry;

/* PRIVATE FUNCTIONS **********************************************************/

BOOL
NTAPI
FailSoundSentry(VOID)
{
    /* In case the function can't be found/is unimplemented */
    return FALSE;
}

BOOL
NTAPI
FirstSoundSentry(VOID)
{
    /* Not done in vista */
    return TRUE;
}

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(BaseSrvSoundSentryNotification)
{
    /* Call the API and see if it succeeds */
    return STATUS_ACCESS_DENIED;
}

/* EOF */

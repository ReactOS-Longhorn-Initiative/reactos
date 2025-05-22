/*
 * PROJECT:     ReactOS Window Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     RWM DWM Command Compatible LPC Messages
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/*
 * One of the problems with trying to name these is that it's possible for us to
 * know all of the internal names. Debug builds of windows can get us a good chunk of them
 * But it's a little more trouble than it worth.
 * 
 * These operations can be dispatched through out different modules to the APIPORT
 * of dwm.exe/uxss.exe. While the service has a limited number of commands
 * this is more universal.
 */

/* 
 * Targetting Longhorn 5112
 * Almost all of these operations with be Pumped through the MilMessagePump.
 */
typedef enum _RWM_COMMANDS
{
    RWMCMD_NOTIFY_SETTINGS_CHANGE =     0x4000000A,
    RWMCMD_REDIR_STARTUP =              0x40000026,
    RWMCMD_REDIR_CHANGESETTINGS =       0x40000034,
    RWMCMD_REDIR_TERMINATEORPHAN =      0x40000042,
 
} RWM_COMMANDS, *PRWM_COMMANDS;

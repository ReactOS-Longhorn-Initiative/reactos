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
    RWMCMD_REDIR_SHUTDOWN =                         0x01,
    RWMCMD_RX_GETSHAREDSURFACE =                    0x02,
    RWMCMD_RX_UPDATESHAREDSURF =                    0x03,
    RWMCMD_REDIR_CREATEWINDOW =                     0x05,
    RWMCMD_REDIR_DESTROYWINDOW =                    0x06,
    RWMCMD_REDIR_DIRTYWINDOW =                      0x07,
    RWMCMD_REDIR_ZORDERWINDOW =                     0x08,
    RWMCMD_REDIR_UPDATESPRITE =                     0x09,
    RWMCMD_REDIR_SHOWWINDOW =                       0x0A,
    RWMCMD_REDIR_ICONCHANGE =                       0x0B,
    RWMCMD_REDIR_TEXTCHANGE =                       0x0C,
    RWMCMD_REDIR_STYLECHANGE =                      0x0D,
    RWMCMD_REDIR_ACTIVATIONCHANGE  =                0x0E,
    RWMCMD_REDIR_DESKTOPCHANGE =                    0x0F,
    RWMCMD_REDIR_SHELLWINDOWCHANGE =                0x10,
    RWMCMD_REDIR_POWERCHANGE =                      0x11,
    RWMCMD_REDIR_CHILDCREATE =                      0x12,
    RWMCMD_REDIR_CHILDLINK =                        0x13,
    RWMCMD_REDIR_CHILDUNLINK =                      0x14,
    RWMCMD_REDIR_CHILDDESTROY =                     0x15,
    RWMCMD_REDIR_CHILDMOVESIZE =                    0x16,
    RWMCMD_REDIR_CHILDSTYLECHANGE =                 0x17,
    RWMCMD_REDIR_CHILDCLIPRGNCHANGE =               0x18,
    RWMCMD_REDIR_HITTESTQUERY =                     0x19,
    RWMCMD_REDIR_MOUSELEAVEWINDOW =                 0x1A,
    RWMCMD_REDIR_CHANGEWINDOWRELATIVE =             0x1B,
    RWMCMD_REDIR_CLIENTAREABLURCHANGEX =            0x1C,
    RWMCMD_REDIR_NCRENDERINGCHANGE =                0x1E,
    RWMCMD_REDIR_REGISTERTHUMBNAIL =                0x1F,
    RWMCMD_REDIR_UPDATETHUMBNAILPROPERTIES =        0x20,
    RWMCMD_REDIR_UNREGISTERTHUMBNAIL =              0x21,
    RWMCMD_CAPTURE_SCREENBITS =                     0x27,
} RWM_COMMANDS, *PRWM_COMMANDS;

/*
 * Internal DWM / MIL bring-up state (Vista Longhorn 5048-style).
 */
#pragma once

struct _EPROCESS;
struct _PORT_MESSAGE;

extern BOOLEAN gfbDwmCompositing;
extern struct _EPROCESS *gpepDwm;

BOOLEAN FASTCALL DwmIsDwmClientProcess(VOID);

VOID FASTCALL IntDwmSendLpcDatagram(_In_ struct _PORT_MESSAGE *Msg);

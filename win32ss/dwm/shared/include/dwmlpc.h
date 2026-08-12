#pragma once

#define MAX_MESSAGE_DATA   (0x130)

typedef struct _LPC_MAX_MESSAGE
{
    PORT_MESSAGE Header;
    ULONG Message;
    ULONG Status;
    BYTE Data[MAX_MESSAGE_DATA - sizeof(ULONG) - sizeof(ULONG)];
} LPC_MAX_MESSAGE, *PLPC_MAX_MESSAGE;

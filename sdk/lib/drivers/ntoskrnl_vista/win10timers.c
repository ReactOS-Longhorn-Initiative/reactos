
#include <ntdef.h>
#include <ntifs.h>
#include <debug.h>

#ifdef _NTOSKRNL_

#define TAG_TIMER2 'RTIM'

typedef
VOID
NTAPI
EXT_CALLBACK(
  _In_ PEX_TIMER Timer,
  _In_opt_ PVOID Context);

/* EX_TIMER == _KTIMER2 ?? LOL */
PEX_TIMER
NTAPI
KiInitializeTimer2(_In_opt_ PEXT_CALLBACK Callback,
                   _In_opt_ PVOID CallbackContext,
                   _In_ ULONG Attributes)//this is definietly not following what windows does 1:1
{
    PKTIMER2 Timer;
    Timer = ExAllocatePoolWithTag(NonPagedPool, sizeof(KTIMER2), TAG_TIMER2);
    if (Attributes)
    {
        Timer->Header.Lock = 0;
        Timer->Header.SignalState = 0;
        Timer->Header.Type = ((Attributes & 0x80000000) == 0) | 0x18; //TODO expand on me why
        InitializeListHead(&(Timer->Header.WaitListHead));
        (EXT_CALLBACK*)Timer->Callback = Callback;
        Timer->CallbackContext = CallbackContext;
        Timer->DisableCallback = 0;
        Timer->DisableContext = 0;
        Timer->TypeFlags = Attributes & 0xE; /// offset into NoWake+

    }

    //TODO: deal with collectionindex, should be based off of attributes.
    return (PEX_TIMER)Timer;
}

/*
 * I have put in the _KTIMER2 strucutre in the NDK up to windows 10 22H2.
 * I assume EX_TIMER is exposed for the sake of naming but that's kind of stupid in general.
 * This will work very simply: calling into intneral functions that expose things as KTIMER2
 * - This is due to typedef btw.
 * Otherwise this works similarly to legacy timerobj.c in ros `KeSetTimer` and friends
 */

/*
 *  @implemented - Win10
 *  However... i need to know if anything calls the functions below this
 */
NTKRNLVISTAAPI
PEX_TIMER
NTAPI
ExAllocateTimer(_In_opt_ PEXT_CALLBACK Callback,
                _In_opt_ PVOID CallbackContext,
                _In_ ULONG Attributes)
{

	return KiInitializeTimer2(Callback, CallbackContext, Attributes);
}

NTKRNLVISTAAPI
BOOLEAN
NTAPI
ExCancelTimer(_Inout_ PEX_TIMER Timer,
              _In_opt_ PEXT_CANCEL_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
	return TRUE;
}

NTKRNLVISTAAPI
BOOLEAN
NTAPI
ExDeleteTimer(_In_ PEX_TIMER Timer,
              _In_ BOOLEAN Cancel,
              _In_ BOOLEAN Wait,
              _In_ PEXT_DELETE_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
	return Cancel;
}

NTKRNLVISTAAPI
BOOLEAN
NTAPI
ExSetTimer(_In_ PEX_TIMER Timer,
           _In_ LONGLONG DueTime,
           _In_ LONGLONG Period,
           _In_opt_ PEXT_SET_PARAMETERS Parameters)
{
    UNIMPLEMENTED;
    DPRINT1("Contact le dark fire\n");
    __debugbreak();
	return FALSE;
}

#endif
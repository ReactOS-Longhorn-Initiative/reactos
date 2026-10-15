#include "k32_vista.h"

typedef void (CALLBACK *PTP_IO_CALLBACK)(PTP_CALLBACK_INSTANCE,void*,void*,IO_STATUS_BLOCK*,PTP_IO);

extern BOOL WINAPI DECLSPEC_HOTPATCH TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                                           TP_CALLBACK_ENVIRON *environment );


/***********************************************************************
 *           TrySubmitThreadpoolCallback   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TrySubmitThreadpoolCallback( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                                           TP_CALLBACK_ENVIRON *environment )
{
    NTSTATUS status = TpSimpleTryPost( callback, userdata, environment );
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }

    return TRUE;
}

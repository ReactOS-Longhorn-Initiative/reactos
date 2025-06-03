#include "ntdll_vista.h"

PVOID LdrpHeap;
#include <debug.h>


/* These APIs are very commonly used in modern apps + needed for kernelbase, But these stubs will work for now. */
NTSTATUS WINAPI LdrGetDllDirectory( UNICODE_STRING *dir )
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS WINAPI LdrSetDllDirectory( const UNICODE_STRING *dir )
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LdrAddDllDirectory( const UNICODE_STRING *dir, void **cookie )
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS WINAPI LdrRemoveDllDirectory( void *cookie )
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LdrSetDefaultDllDirectories( ULONG flags )
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *           RtlGetProductInfo    (NTDLL.@)
 *
 * Gives info about the current Windows product type, in a format compatible
 * with the given Windows version
 *
 * Returns TRUE if the input is valid, FALSE otherwise
 */
BOOLEAN WINAPI RtlGetProductInfo(DWORD dwOSMajorVersion, DWORD dwOSMinorVersion, DWORD dwSpMajorVersion,
                                 DWORD dwSpMinorVersion, PDWORD pdwReturnedProductType)
{
    DPRINT1("(%ld, %ld, %ld, %ld, %p)\n", dwOSMajorVersion, dwOSMinorVersion,
          dwSpMajorVersion, dwSpMinorVersion, pdwReturnedProductType);

    if (!pdwReturnedProductType)
        return FALSE;

    if (dwOSMajorVersion < 6)
    {
        *pdwReturnedProductType = PRODUCT_UNDEFINED;
        return FALSE;
    }

    *pdwReturnedProductType = PRODUCT_ULTIMATE_N;

    return TRUE;
}

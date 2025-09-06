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

 
 

/***********************************************************************
 *           RtlGetProductInfo    (NTDLL.@)
 *
 * Gives info about the current Windows product type, in a format compatible
 * with the given Windows version
 *
 * Returns TRUE if the input is valid, FALSE otherwise
 */
BOOLEAN 
WINAPI 
RtlGetProductInfo(
	DWORD dwOSMajorVersion, 
	DWORD dwOSMinorVersion, 
	DWORD dwSpMajorVersion,
    DWORD dwSpMinorVersion, 
	PDWORD pdwReturnedProductType
)
{
    RTL_OSVERSIONINFOEXW VersionInformation;
	
	VersionInformation.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOEXW);
	
	RtlGetVersion((PRTL_OSVERSIONINFOW)&VersionInformation);

    if (!pdwReturnedProductType)
        return FALSE;

    if (VersionInformation.wProductType == VER_NT_WORKSTATION)
	{
		if(VersionInformation.wSuiteMask == VER_SUITE_PERSONAL)
			*pdwReturnedProductType = PRODUCT_HOME_PREMIUM;
		else
			*pdwReturnedProductType = PRODUCT_ULTIMATE;
	}else{
		if(VersionInformation.wSuiteMask == VER_SUITE_BLADE)
			*pdwReturnedProductType = PRODUCT_WEB_SERVER;
		if(VersionInformation.wSuiteMask == VER_SUITE_COMPUTE_SERVER)
			*pdwReturnedProductType = PRODUCT_CLUSTER_SERVER;
		if(VersionInformation.wSuiteMask == VER_SUITE_DATACENTER)
			*pdwReturnedProductType = PRODUCT_DATACENTER_SERVER;
		if(VersionInformation.wSuiteMask == VER_SUITE_ENTERPRISE)
			*pdwReturnedProductType = PRODUCT_ENTERPRISE_SERVER;
		if(VersionInformation.wSuiteMask == VER_SUITE_SMALLBUSINESS)
			*pdwReturnedProductType = PRODUCT_SMALLBUSINESS_SERVER;		
		if(VersionInformation.wSuiteMask == VER_SUITE_SMALLBUSINESS_RESTRICTED)
			*pdwReturnedProductType = PRODUCT_SB_SOLUTION_SERVER;	
		if(VersionInformation.wSuiteMask == VER_SUITE_STORAGE_SERVER)
			*pdwReturnedProductType = PRODUCT_STORAGE_ENTERPRISE_SERVER;		
		if(VersionInformation.wSuiteMask == VER_SUITE_WH_SERVER)
			*pdwReturnedProductType = PRODUCT_HOME_PREMIUM_SERVER;			
	}        

    return TRUE;
}

struct _KUSER_SHARED_DATA *user_shared_data = (void *)0x7ffe0000;

BOOL WINAPI RtlQueryUnbiasedInterruptTime(ULONGLONG *time)
{
    ULONG high, low;

    if (!time)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus( STATUS_INVALID_PARAMETER );
        return FALSE;
    }

    do
    {
        high = user_shared_data->InterruptTime.High1Time;
        low = user_shared_data->InterruptTime.LowPart;
    }
    while (high != user_shared_data->InterruptTime.High2Time);
    /* FIXME: should probably subtract InterruptTimeBias */
    *time = (ULONGLONG)high << 32 | low;
    return TRUE;
}


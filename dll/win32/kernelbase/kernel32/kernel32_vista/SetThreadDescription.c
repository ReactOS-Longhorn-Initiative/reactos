
#include "k32_vista.h"
#define NDEBUG
#include <debug.h>

#undef TRACE
#define TRACE DPRINT

HRESULT


WINAPI


DECLSPEC_HOTPATCH


GetThreadDescription(


    _In_ HANDLE hThread,


    _Outptr_result_z_ PWSTR* ppszThreadDescription)


{


    PTHREAD_NAME_INFORMATION NameInfo = NULL;


    PVOID ptr;


    NTSTATUS Status;


    ULONG Length;





    *ppszThreadDescription = NULL;





    /* Since there may be concurrent SetThreadDescription() invocations being


     * executed, we need to loop over the buffer allocation size until we can


     * successfully capture the thread name. */


    Length = 0;


    Status = NtQueryInformationThread(hThread, ThreadNameInformation, NULL, 0, &Length);


    if (Status != STATUS_BUFFER_TOO_SMALL)


        return HRESULT_FROM_NT(Status);





    while (NT_SUCCESS(Status))


    {


        /* (Re-)allocate the information buffer */


        ptr = (NameInfo ? LocalReAlloc(NameInfo, Length, LMEM_MOVEABLE)


                        : LocalAlloc(LMEM_FIXED, Length));


        if (!ptr)


        {


            Status = STATUS_NO_MEMORY;


            break;


        }


        NameInfo = ptr;





        Status = NtQueryInformationThread(hThread, ThreadNameInformation, NameInfo, Length, &Length);





        /* Loop again only if we get buffer-too-small types of errors;


         * otherwise, break the loop: any error will be handled below. */


        if (Status != STATUS_BUFFER_OVERFLOW &&


            Status != STATUS_INFO_LENGTH_MISMATCH &&


            Status != STATUS_BUFFER_TOO_SMALL)


        {


            break;


        }


        Status = STATUS_SUCCESS;


    }


    if (!NT_SUCCESS(Status))


    {


        LocalFree(NameInfo);


        return HRESULT_FROM_NT(Status);


    }





    /*


     * We already have a suitable memory buffer, containing a UNICODE_STRING


     * followed by the actual string data. Just move the string back to the


     * beginning of the buffer and resize it down. Since sizeof(UNICODE_STRING)


     * is larger than sizeof(WCHAR), we also have space for the NUL-terminator.


     */


    *ppszThreadDescription = (PWSTR)NameInfo;


    Length = NameInfo->ThreadName.Length;


    if (Length)


        RtlMoveMemory(*ppszThreadDescription, NameInfo->ThreadName.Buffer, Length);


    (*ppszThreadDescription)[Length / sizeof(WCHAR)] = UNICODE_NULL;





    /* Resize down the buffer. If the call fails, the old buffer


     * is still valid, but is larger than what is necessary. */


    ptr = LocalReAlloc(*ppszThreadDescription, Length + sizeof(WCHAR), LMEM_FIXED);


    if (ptr)


        *ppszThreadDescription = ptr;





    return HRESULT_FROM_NT(Status);


}





HRESULT


WINAPI


DECLSPEC_HOTPATCH


SetThreadDescription(


    _In_ HANDLE hThread,


    _In_ PCWSTR lpThreadDescription)


{


    THREAD_NAME_INFORMATION NameInfo;


    NTSTATUS Status;





    Status = RtlInitUnicodeStringEx(&NameInfo.ThreadName, lpThreadDescription);


    if (NT_SUCCESS(Status))


        Status = NtSetInformationThread(hThread, ThreadNameInformation, &NameInfo, sizeof(NameInfo));


// else Status = STATUS_INVALID_PARAMETER;


    return HRESULT_FROM_NT(Status);


}
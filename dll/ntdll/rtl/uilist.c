/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode DLL
 * FILE:            lib/ntdll/rtl/uilist.c
 * PURPOSE:         RTL UI to API network computers list conversion.
 *                  Helper for NETAPI32.DLL
 * PROGRAMMERS:     Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlConvertUiListToApiList(
    IN PUNICODE_STRING UiList,
    OUT PUNICODE_STRING ApiList,
    IN BOOLEAN SpaceAsSeparator)
{
    DPRINT1("RtlConvertUiListToApiList(%wZ, 0x%p, %s) called\n",
            UiList, &ApiList, SpaceAsSeparator ? "true" : "false");
    UNIMPLEMENTED;
    /*
     * Experiments show that returning a success code but setting the
     * ApiList length to zero is better than returning a failure code.
     */
    RtlInitEmptyUnicodeString(ApiList, NULL, 0);
    return STATUS_SUCCESS;
}

/* generically adds an ACE to an ACL */
static NTSTATUS add_access_ace(PACL pAcl, DWORD dwAceRevision, DWORD dwAceFlags,
                               DWORD dwAccessMask, PSID pSid, DWORD dwAceType)
{
    ACE_HEADER *pAceHeader;
    DWORD dwLengthSid;
    DWORD dwAceSize;
    DWORD *pAccessMask;
    DWORD *pSidStart;

    if (!RtlValidSid(pSid))
        return STATUS_INVALID_SID;

    if (pAcl->AclRevision > MAX_ACL_REVISION || dwAceRevision > MAX_ACL_REVISION)
        return STATUS_REVISION_MISMATCH;

    if (!RtlValidAcl(pAcl))
        return STATUS_INVALID_ACL;

    if (!RtlFirstFreeAce(pAcl, (PACE *)&pAceHeader))
        return STATUS_INVALID_ACL;

    if (!pAceHeader)
        return STATUS_ALLOTTED_SPACE_EXCEEDED;

    /* calculate generic size of the ACE */
    dwLengthSid = RtlLengthSid(pSid);
    dwAceSize = sizeof(ACE_HEADER) + sizeof(DWORD) + dwLengthSid;
    if ((char *)pAceHeader + dwAceSize > (char *)pAcl + pAcl->AclSize)
        return STATUS_ALLOTTED_SPACE_EXCEEDED;

    /* fill the new ACE */
    pAceHeader->AceType = dwAceType;
    pAceHeader->AceFlags = dwAceFlags;
    pAceHeader->AceSize = dwAceSize;

    /* skip past the ACE_HEADER of the ACE */
    pAccessMask = (DWORD *)(pAceHeader + 1);
    *pAccessMask = dwAccessMask;

    /* skip past ACE->Mask */
    pSidStart = pAccessMask + 1;
    RtlCopySid(dwLengthSid, pSidStart, pSid);

    pAcl->AclRevision = max(pAcl->AclRevision, dwAceRevision);
    pAcl->AceCount++;

    return STATUS_SUCCESS;
}

typedef struct _RTL_TIME_DYNAMIC_ZONE_INFORMATION
{
    LONG Bias;
    WCHAR StandardName[32];
    TIME_FIELDS StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[32];
    TIME_FIELDS DaylightDate;
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[128];
    BOOLEAN DynamicDaylightTimeDisabled;
} RTL_DYNAMIC_TIME_ZONE_INFORMATION, *PRTL_DYNAMIC_TIME_ZONE_INFORMATION;


NTSTATUS WINAPI RtlQueryDynamicTimeZoneInformation(RTL_DYNAMIC_TIME_ZONE_INFORMATION *ret)
{
    return NtQuerySystemInformation( 102, ret, sizeof(*ret), NULL ); //SystemDynamicTimeZoneInformation
}

NTSTATUS WINAPI RtlAddMandatoryAce(
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD dwAceFlags,
    IN DWORD dwMandatoryFlags,
    IN UCHAR dwAceType,
    IN PSID pSid)
{
 
    if (dwAceType != SYSTEM_MANDATORY_LABEL_ACE_TYPE)
        return STATUS_INVALID_PARAMETER;
    if (dwMandatoryFlags & ~SYSTEM_MANDATORY_LABEL_VALID_MASK)
        return STATUS_INVALID_PARAMETER;

    return add_access_ace(pAcl, dwAceRevision, dwAceFlags, dwMandatoryFlags, pSid, dwAceType);
}

/* EOF */

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/sec.c
 * PURPOSE:         Security descriptor functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Steven Edwards ( Steven_Ed4153@yahoo.com )
 *                  Andrew Greenwood ( silverblade_uk@hotmail.com )
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <advapi32.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/*
 * @implemented
 */
BOOL
WINAPI
GetSecurityDescriptorControl(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                             PSECURITY_DESCRIPTOR_CONTROL pControl,
                             LPDWORD lpdwRevision)
{
    NTSTATUS Status;

    Status = RtlGetControlSecurityDescriptor(pSecurityDescriptor,
                                             pControl,
                                             (PULONG)lpdwRevision);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetSecurityDescriptorDacl(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                          LPBOOL lpbDaclPresent,
                          PACL *pDacl,
                          LPBOOL lpbDaclDefaulted)
{
    BOOLEAN DaclPresent;
    BOOLEAN DaclDefaulted;
    NTSTATUS Status;

    Status = RtlGetDaclSecurityDescriptor(pSecurityDescriptor,
                                          &DaclPresent,
                                          pDacl,
                                          &DaclDefaulted);
    *lpbDaclPresent = (BOOL)DaclPresent;
    *lpbDaclDefaulted = (BOOL)DaclDefaulted;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetSecurityDescriptorGroup(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                           PSID *pGroup,
                           LPBOOL lpbGroupDefaulted)
{
    BOOLEAN GroupDefaulted;
    NTSTATUS Status;

    Status = RtlGetGroupSecurityDescriptor(pSecurityDescriptor,
                                           pGroup,
                                           &GroupDefaulted);
    *lpbGroupDefaulted = (BOOL)GroupDefaulted;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetSecurityDescriptorOwner(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                           PSID *pOwner,
                           LPBOOL lpbOwnerDefaulted)
{
    BOOLEAN OwnerDefaulted;
    NTSTATUS Status;

    Status = RtlGetOwnerSecurityDescriptor(pSecurityDescriptor,
                                           pOwner,
                                           &OwnerDefaulted);
    *lpbOwnerDefaulted = (BOOL)OwnerDefaulted;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetSecurityDescriptorRMControl(PSECURITY_DESCRIPTOR SecurityDescriptor,
                               PUCHAR RMControl)
{
    if (!RtlGetSecurityDescriptorRMControl(SecurityDescriptor,
                                           RMControl))
        return ERROR_INVALID_DATA;

    return ERROR_SUCCESS;
}


/*
 * @implemented (SECURITY DESCRIPTORS ARE BROKEN WITH WIN7+ APPS)
 */
BOOL
WINAPI
GetSecurityDescriptorSacl(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                          LPBOOL lpbSaclPresent,
                          PACL *pSacl,
                          LPBOOL lpbSaclDefaulted)
{

    #if 0
    BOOLEAN SaclPresent;
    BOOLEAN SaclDefaulted;
    NTSTATUS Status;

    Status = RtlGetSaclSecurityDescriptor(pSecurityDescriptor,
                                          &SaclPresent,
                                          pSacl,
                                          &SaclDefaulted);
    *lpbSaclPresent = (BOOL)SaclPresent;
    *lpbSaclDefaulted = (BOOL)SaclDefaulted;

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
    #endif
    /*
     * okay this is kinda weird you might think, but this is actually required for chrome
     * After Chrome 50, the App started to call on this API to handle some security stuff
     * and that entire line of garabge is just destroyed with vista+ ROS. THis is a work around
     * TODO: FIXME please :(
     */
    SetLastError(RtlNtStatusToDosError(1));
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsValidSecurityDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    BOOLEAN Result;

    Result = RtlValidSecurityDescriptor (pSecurityDescriptor);
    if (Result == FALSE)
        SetLastError(RtlNtStatusToDosError(STATUS_INVALID_SECURITY_DESCR));

    return (BOOL)Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
MakeAbsoluteSD2(IN OUT PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
                OUT LPDWORD lpdwBufferSize)
{
    NTSTATUS Status;

    Status = RtlSelfRelativeToAbsoluteSD2(pSelfRelativeSecurityDescriptor,
                                          lpdwBufferSize);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
MakeSelfRelativeSD(PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
                   PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
                   LPDWORD lpdwBufferLength)
{
    NTSTATUS Status;

    Status = RtlAbsoluteToSelfRelativeSD(pAbsoluteSecurityDescriptor,
                                         pSelfRelativeSecurityDescriptor,
                                         (PULONG)lpdwBufferLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetSecurityDescriptorControl(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                             SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
                             SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet)
{
    NTSTATUS Status;

    Status = RtlSetControlSecurityDescriptor(pSecurityDescriptor,
                                             ControlBitsOfInterest,
                                             ControlBitsToSet);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetSecurityDescriptorDacl(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                          BOOL bDaclPresent,
                          PACL pDacl,
                          BOOL bDaclDefaulted)
{
    NTSTATUS Status;

    Status = RtlSetDaclSecurityDescriptor(pSecurityDescriptor,
                                          bDaclPresent,
                                          pDacl,
                                          bDaclDefaulted);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetSecurityDescriptorGroup(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                           PSID pGroup,
                           BOOL bGroupDefaulted)
{
    NTSTATUS Status;

    Status = RtlSetGroupSecurityDescriptor(pSecurityDescriptor,
                                           pGroup,
                                           bGroupDefaulted);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetSecurityDescriptorOwner(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                           PSID pOwner,
                           BOOL bOwnerDefaulted)
{
    NTSTATUS Status;

    Status = RtlSetOwnerSecurityDescriptor(pSecurityDescriptor,
                                           pOwner,
                                           bOwnerDefaulted);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
SetSecurityDescriptorRMControl(PSECURITY_DESCRIPTOR SecurityDescriptor,
                               PUCHAR RMControl)
{
    RtlSetSecurityDescriptorRMControl(SecurityDescriptor,
                                      RMControl);

    return ERROR_SUCCESS;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetSecurityDescriptorSacl(PSECURITY_DESCRIPTOR pSecurityDescriptor,
                          BOOL bSaclPresent,
                          PACL pSacl,
                          BOOL bSaclDefaulted)
{
    NTSTATUS Status;

    Status = RtlSetSaclSecurityDescriptor(pSecurityDescriptor,
                                          bSaclPresent,
                                          pSacl,
                                          bSaclDefaulted);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
VOID
WINAPI
QuerySecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                        OUT LPDWORD DesiredAccess)
{
    *DesiredAccess = 0;

    if (SecurityInformation & (OWNER_SECURITY_INFORMATION |
                               GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION))
    {
        *DesiredAccess |= READ_CONTROL;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
}


/*
 * @implemented
 */
VOID
WINAPI
SetSecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                      OUT LPDWORD DesiredAccess)
{
    *DesiredAccess = 0;

    if (SecurityInformation & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
        *DesiredAccess |= WRITE_OWNER;

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
        *DesiredAccess |= WRITE_DAC;

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
ConvertToAutoInheritPrivateObjectSecurity(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                          IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
                                          OUT PSECURITY_DESCRIPTOR* NewSecurityDescriptor,
                                          IN GUID* ObjectType,
                                          IN BOOLEAN IsDirectoryObject,
                                          IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @implemented
 */
DWORD
WINAPI
BuildSecurityDescriptorW(IN PTRUSTEE_W pOwner  OPTIONAL,
                         IN PTRUSTEE_W pGroup  OPTIONAL,
                         IN ULONG cCountOfAccessEntries,
                         IN PEXPLICIT_ACCESS_W pListOfAccessEntries  OPTIONAL,
                         IN ULONG cCountOfAuditEntries,
                         IN PEXPLICIT_ACCESS_W pListOfAuditEntries  OPTIONAL,
                         IN PSECURITY_DESCRIPTOR pOldSD  OPTIONAL,
                         OUT PULONG pSizeNewSD,
                         OUT PSECURITY_DESCRIPTOR* pNewSD)
{
    SECURITY_DESCRIPTOR AbsSD;
    PSECURITY_DESCRIPTOR RelativeSD = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    PSID pOwnerSid = NULL, pGroupSid = NULL;
    PACL pDacl = NULL, pSacl = NULL;
    PACL pOldDacl = NULL, pOldSacl = NULL;
    BOOL bDaclPresent = FALSE, bSaclPresent = FALSE;
    BOOL bDaclDefaulted = FALSE, bSaclDefaulted = FALSE;
    DWORD dwSDLength;

    TRACE("(%p,%p,%lu,%p,%lu,%p,%p,%p,%p)\n", pOwner, pGroup, cCountOfAccessEntries,
          pListOfAccessEntries, cCountOfAuditEntries, pListOfAuditEntries, 
          pOldSD, pSizeNewSD, pNewSD);

    if (!pSizeNewSD || !pNewSD)
        return ERROR_INVALID_PARAMETER;

    *pNewSD = NULL;
    *pSizeNewSD = 0;

    /* Initialize the security descriptor */
    if (!InitializeSecurityDescriptor(&AbsSD, SECURITY_DESCRIPTOR_REVISION))
    {
        dwStatus = GetLastError();
        goto Cleanup;
    }

    /* Extract information from the old security descriptor if provided */
    if (pOldSD)
    {
        PSID pOldOwner, pOldGroup;
        BOOL bOwnerDefaulted, bGroupDefaulted;

        /* Get owner */
        if (!GetSecurityDescriptorOwner(pOldSD, &pOldOwner, &bOwnerDefaulted))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }

        /* Get group */
        if (!GetSecurityDescriptorGroup(pOldSD, &pOldGroup, &bGroupDefaulted))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }

        /* Get DACL */
        if (!GetSecurityDescriptorDacl(pOldSD, &bDaclPresent, &pOldDacl, &bDaclDefaulted))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }

        /* Get SACL */
        if (!GetSecurityDescriptorSacl(pOldSD, &bSaclPresent, &pOldSacl, &bSaclDefaulted))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }

        /* Use old owner if no new owner specified */
        if (!pOwner && pOldOwner)
        {
            pOwnerSid = pOldOwner;
        }

        /* Use old group if no new group specified */
        if (!pGroup && pOldGroup)
        {
            pGroupSid = pOldGroup;
        }
    }

    /* Get SID from owner trustee */
    if (pOwner)
    {
        if (pOwner->TrusteeForm == TRUSTEE_IS_SID)
        {
            pOwnerSid = (PSID)pOwner->ptstrName;
        }
        else if (pOwner->TrusteeForm == TRUSTEE_IS_NAME)
        {
            /* Convert name to SID - simplified implementation */
            dwStatus = ERROR_NOT_SUPPORTED;
            goto Cleanup;
        }
    }

    /* Get SID from group trustee */
    if (pGroup)
    {
        if (pGroup->TrusteeForm == TRUSTEE_IS_SID)
        {
            pGroupSid = (PSID)pGroup->ptstrName;
        }
        else if (pGroup->TrusteeForm == TRUSTEE_IS_NAME)
        {
            /* Convert name to SID - simplified implementation */
            dwStatus = ERROR_NOT_SUPPORTED;
            goto Cleanup;
        }
    }

    /* Set owner */
    if (pOwnerSid)
    {
        if (!SetSecurityDescriptorOwner(&AbsSD, pOwnerSid, FALSE))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }
    }

    /* Set group */
    if (pGroupSid)
    {
        if (!SetSecurityDescriptorGroup(&AbsSD, pGroupSid, FALSE))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }
    }

    /* Build DACL from access entries */
    if (cCountOfAccessEntries > 0 && pListOfAccessEntries)
    {
        dwStatus = SetEntriesInAclW(cCountOfAccessEntries,
                                    pListOfAccessEntries,
                                    pOldDacl,
                                    &pDacl);
        if (dwStatus != ERROR_SUCCESS)
            goto Cleanup;

        if (!SetSecurityDescriptorDacl(&AbsSD, TRUE, pDacl, FALSE))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }
    }
    else if (bDaclPresent && pOldDacl)
    {
        /* Use old DACL if no new entries */
        if (!SetSecurityDescriptorDacl(&AbsSD, TRUE, pOldDacl, bDaclDefaulted))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }
    }

    /* Build SACL from audit entries */
    if (cCountOfAuditEntries > 0 && pListOfAuditEntries)
    {
        dwStatus = SetEntriesInAclW(cCountOfAuditEntries,
                                    pListOfAuditEntries,
                                    pOldSacl,
                                    &pSacl);
        if (dwStatus != ERROR_SUCCESS)
            goto Cleanup;

        if (!SetSecurityDescriptorSacl(&AbsSD, TRUE, pSacl, FALSE))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }
    }
    else if (bSaclPresent && pOldSacl)
    {
        /* Use old SACL if no new entries */
        if (!SetSecurityDescriptorSacl(&AbsSD, TRUE, pOldSacl, bSaclDefaulted))
        {
            dwStatus = GetLastError();
            goto Cleanup;
        }
    }

    /* Convert to self-relative format */
    dwSDLength = 0;
    if (!MakeSelfRelativeSD(&AbsSD, NULL, &dwSDLength) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        dwStatus = GetLastError();
        goto Cleanup;
    }

    RelativeSD = LocalAlloc(LMEM_FIXED, dwSDLength);
    if (!RelativeSD)
    {
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if (!MakeSelfRelativeSD(&AbsSD, RelativeSD, &dwSDLength))
    {
        dwStatus = GetLastError();
        goto Cleanup;
    }

    /* Success */
    *pNewSD = RelativeSD;
    *pSizeNewSD = dwSDLength;
    RelativeSD = NULL; /* Don't free on cleanup */

Cleanup:
    if (RelativeSD)
        LocalFree(RelativeSD);
    
    if (pDacl && pDacl != pOldDacl)
        LocalFree(pDacl);
        
    if (pSacl && pSacl != pOldSacl)
        LocalFree(pSacl);

    return dwStatus;
}


/*
 * @implemented
 */
DWORD
WINAPI
BuildSecurityDescriptorA(IN PTRUSTEE_A pOwner  OPTIONAL,
                         IN PTRUSTEE_A pGroup  OPTIONAL,
                         IN ULONG cCountOfAccessEntries,
                         IN PEXPLICIT_ACCESS_A pListOfAccessEntries  OPTIONAL,
                         IN ULONG cCountOfAuditEntries,
                         IN PEXPLICIT_ACCESS_A pListOfAuditEntries  OPTIONAL,
                         IN PSECURITY_DESCRIPTOR pOldSD  OPTIONAL,
                         OUT PULONG pSizeNewSD,
                         OUT PSECURITY_DESCRIPTOR* pNewSD)
{
    PTRUSTEE_W pOwnerW = NULL, pGroupW = NULL;
    PEXPLICIT_ACCESS_W pListOfAccessEntriesW = NULL, pListOfAuditEntriesW = NULL;
    DWORD dwStatus;

    TRACE("(%p,%p,%lu,%p,%lu,%p,%p,%p,%p)\n", pOwner, pGroup, cCountOfAccessEntries,
          pListOfAccessEntries, cCountOfAuditEntries, pListOfAuditEntries, 
          pOldSD, pSizeNewSD, pNewSD);

    /* Convert trustees from A to W */
    if (pOwner)
    {
        pOwnerW = LocalAlloc(LMEM_FIXED, sizeof(TRUSTEE_W));
        if (!pOwnerW)
            return ERROR_NOT_ENOUGH_MEMORY;
            
        /* For simplicity, only support SID trustees for now */
        if (pOwner->TrusteeForm == TRUSTEE_IS_SID)
        {
            *pOwnerW = *(PTRUSTEE_W)pOwner; /* SID is the same in A and W */
        }
        else
        {
            LocalFree(pOwnerW);
            return ERROR_NOT_SUPPORTED;
        }
    }

    if (pGroup)
    {
        pGroupW = LocalAlloc(LMEM_FIXED, sizeof(TRUSTEE_W));
        if (!pGroupW)
        {
            if (pOwnerW) LocalFree(pOwnerW);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
            
        /* For simplicity, only support SID trustees for now */
        if (pGroup->TrusteeForm == TRUSTEE_IS_SID)
        {
            *pGroupW = *(PTRUSTEE_W)pGroup; /* SID is the same in A and W */
        }
        else
        {
            if (pOwnerW) LocalFree(pOwnerW);
            LocalFree(pGroupW);
            return ERROR_NOT_SUPPORTED;
        }
    }

    /* Convert access entries - simplified for SID trustees only */
    if (cCountOfAccessEntries > 0 && pListOfAccessEntries)
    {
        pListOfAccessEntriesW = LocalAlloc(LMEM_FIXED, cCountOfAccessEntries * sizeof(EXPLICIT_ACCESS_W));
        if (!pListOfAccessEntriesW)
        {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        for (ULONG i = 0; i < cCountOfAccessEntries; i++)
        {
            pListOfAccessEntriesW[i].grfAccessPermissions = pListOfAccessEntries[i].grfAccessPermissions;
            pListOfAccessEntriesW[i].grfAccessMode = pListOfAccessEntries[i].grfAccessMode;
            pListOfAccessEntriesW[i].grfInheritance = pListOfAccessEntries[i].grfInheritance;
            
            /* Copy trustee - only support SID trustees for now */
            if (pListOfAccessEntries[i].Trustee.TrusteeForm == TRUSTEE_IS_SID)
            {
                pListOfAccessEntriesW[i].Trustee = *(PTRUSTEE_W)&pListOfAccessEntries[i].Trustee;
            }
            else
            {
                dwStatus = ERROR_NOT_SUPPORTED;
                goto Cleanup;
            }
        }
    }

    /* Convert audit entries - simplified for SID trustees only */
    if (cCountOfAuditEntries > 0 && pListOfAuditEntries)
    {
        pListOfAuditEntriesW = LocalAlloc(LMEM_FIXED, cCountOfAuditEntries * sizeof(EXPLICIT_ACCESS_W));
        if (!pListOfAuditEntriesW)
        {
            dwStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        for (ULONG i = 0; i < cCountOfAuditEntries; i++)
        {
            pListOfAuditEntriesW[i].grfAccessPermissions = pListOfAuditEntries[i].grfAccessPermissions;
            pListOfAuditEntriesW[i].grfAccessMode = pListOfAuditEntries[i].grfAccessMode;
            pListOfAuditEntriesW[i].grfInheritance = pListOfAuditEntries[i].grfInheritance;
            
            /* Copy trustee - only support SID trustees for now */
            if (pListOfAuditEntries[i].Trustee.TrusteeForm == TRUSTEE_IS_SID)
            {
                pListOfAuditEntriesW[i].Trustee = *(PTRUSTEE_W)&pListOfAuditEntries[i].Trustee;
            }
            else
            {
                dwStatus = ERROR_NOT_SUPPORTED;
                goto Cleanup;
            }
        }
    }

    /* Call the W version */
    dwStatus = BuildSecurityDescriptorW(pOwnerW, pGroupW,
                                        cCountOfAccessEntries, pListOfAccessEntriesW,
                                        cCountOfAuditEntries, pListOfAuditEntriesW,
                                        pOldSD, pSizeNewSD, pNewSD);

Cleanup:
    if (pOwnerW) LocalFree(pOwnerW);
    if (pGroupW) LocalFree(pGroupW);
    if (pListOfAccessEntriesW) LocalFree(pListOfAccessEntriesW);
    if (pListOfAuditEntriesW) LocalFree(pListOfAuditEntriesW);

    return dwStatus;
}

/* EOF */

NTMARTA NtMartaStatic = { 0 };
static PNTMARTA NtMarta = NULL;

#define FindNtMartaProc(Name)                                                  \
    NtMartaStatic.Name = (PVOID)GetProcAddress(NtMartaStatic.hDllInstance,     \
                                               "Acc" # Name );                 \
    if (NtMartaStatic.Name == NULL)                                            \
    {                                                                          \
        return GetLastError();                                                 \
    }


static DWORD
LoadAndInitializeNtMarta(VOID)
{
    /* this code may be executed simultaneously by multiple threads in case they're
       trying to initialize the interface at the same time, but that's no problem
       because the pointers returned by GetProcAddress will be the same. However,
       only one of the threads will change the NtMarta pointer to the NtMartaStatic
       structure, the others threads will detect that there were other threads
       initializing the structure faster and will release the reference to the
       DLL */

    NtMartaStatic.hDllInstance = LoadLibraryW(L"ntmarta.dll");
    if (NtMartaStatic.hDllInstance == NULL)
    {
        return GetLastError();
    }

#if 0
    FindNtMartaProc(LookupAccountTrustee);
    FindNtMartaProc(LookupAccountName);
    FindNtMartaProc(LookupAccountSid);
    FindNtMartaProc(SetEntriesInAList);
    FindNtMartaProc(ConvertAccessToSecurityDescriptor);
    FindNtMartaProc(ConvertSDToAccess);
    FindNtMartaProc(ConvertAclToAccess);
    FindNtMartaProc(GetAccessForTrustee);
    FindNtMartaProc(GetExplicitEntries);
#endif
    FindNtMartaProc(RewriteGetNamedRights);
    FindNtMartaProc(RewriteSetNamedRights);
    FindNtMartaProc(RewriteGetHandleRights);
    FindNtMartaProc(RewriteSetHandleRights);
    FindNtMartaProc(RewriteSetEntriesInAcl);
    FindNtMartaProc(RewriteGetExplicitEntriesFromAcl);
    FindNtMartaProc(TreeResetNamedSecurityInfo);
    FindNtMartaProc(GetInheritanceSource);
    FindNtMartaProc(FreeIndexArray);

    return ERROR_SUCCESS;
}


DWORD
CheckNtMartaPresent(VOID)
{
    DWORD ErrorCode;

    if (InterlockedCompareExchangePointer((PVOID)&NtMarta,
                                          NULL,
                                          NULL) == NULL)
    {
        /* we're the first one trying to use ntmarta, initialize it and change
           the pointer after initialization */
        ErrorCode = LoadAndInitializeNtMarta();

        if (ErrorCode == ERROR_SUCCESS)
        {
            /* try change the NtMarta pointer */
            if (InterlockedCompareExchangePointer((PVOID)&NtMarta,
                                                  &NtMartaStatic,
                                                  NULL) != NULL)
            {
                /* another thread initialized ntmarta in the meanwhile, release
                   the reference of the dll loaded. */
                FreeLibrary(NtMartaStatic.hDllInstance);
            }
        }
#if DBG
        else
        {
            ERR("Failed to initialize ntmarta.dll! Error: 0x%x\n", ErrorCode);
        }
#endif
    }
    else
    {
        /* ntmarta was already initialized */
        ErrorCode = ERROR_SUCCESS;
    }

    return ErrorCode;
}


VOID
UnloadNtMarta(VOID)
{
    if (InterlockedExchangePointer((PVOID)&NtMarta,
                                   NULL) != NULL)
    {
        FreeLibrary(NtMartaStatic.hDllInstance);
    }
}



/*
 * @implemented
 */
BOOL
WINAPI
CreatePrivateObjectSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
                            PSECURITY_DESCRIPTOR CreatorDescriptor,
                            PSECURITY_DESCRIPTOR *NewDescriptor,
                            BOOL IsDirectoryObject,
                            HANDLE Token,
                            PGENERIC_MAPPING GenericMapping)
{
    NTSTATUS Status;

    Status = RtlNewSecurityObject(ParentDescriptor,
                                  CreatorDescriptor,
                                  NewDescriptor,
                                  IsDirectoryObject,
                                  Token,
                                  GenericMapping);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CreatePrivateObjectSecurityEx(PSECURITY_DESCRIPTOR ParentDescriptor,
                              PSECURITY_DESCRIPTOR CreatorDescriptor,
                              PSECURITY_DESCRIPTOR* NewDescriptor,
                              GUID* ObjectType,
                              BOOL IsContainerObject,
                              ULONG AutoInheritFlags,
                              HANDLE Token,
                              PGENERIC_MAPPING GenericMapping)
{
    FIXME("%s() not implemented!\n", __FUNCTION__);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CreatePrivateObjectSecurityWithMultipleInheritance(PSECURITY_DESCRIPTOR ParentDescriptor,
                                                   PSECURITY_DESCRIPTOR CreatorDescriptor,
                                                   PSECURITY_DESCRIPTOR* NewDescriptor,
                                                   GUID** ObjectTypes,
                                                   ULONG GuidCount,
                                                   BOOL IsContainerObject,
                                                   ULONG AutoInheritFlags,
                                                   HANDLE Token,
                                                   PGENERIC_MAPPING GenericMapping)
{
    FIXME("%s() semi-stub\n", __FUNCTION__);
    return CreatePrivateObjectSecurity(ParentDescriptor, CreatorDescriptor, NewDescriptor, IsContainerObject, Token, GenericMapping);
}


/*
 * @implemented
 */
BOOL
WINAPI
DestroyPrivateObjectSecurity(PSECURITY_DESCRIPTOR *ObjectDescriptor)
{
    NTSTATUS Status;

    Status = RtlDeleteSecurityObject(ObjectDescriptor);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetPrivateObjectSecurity(IN PSECURITY_DESCRIPTOR ObjectDescriptor,
                         IN SECURITY_INFORMATION SecurityInformation,
                         OUT PSECURITY_DESCRIPTOR ResultantDescriptor OPTIONAL,
                         IN DWORD DescriptorLength,
                         OUT PDWORD ReturnLength)
{
    NTSTATUS Status;

    /* Call RTL */
    Status = RtlQuerySecurityObject(ObjectDescriptor,
                                    SecurityInformation,
                                    ResultantDescriptor,
                                    DescriptorLength,
                                    ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Success */
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetPrivateObjectSecurity(SECURITY_INFORMATION SecurityInformation,
                         PSECURITY_DESCRIPTOR ModificationDescriptor,
                         PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                         PGENERIC_MAPPING GenericMapping,
                         HANDLE Token)
{
    NTSTATUS Status;

    Status = RtlSetSecurityObject(SecurityInformation,
                                  ModificationDescriptor,
                                  ObjectsSecurityDescriptor,
                                  GenericMapping,
                                  Token);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
ImpersonateAnonymousToken(IN HANDLE ThreadHandle)
{
    NTSTATUS Status;

    Status = NtImpersonateAnonymousToken(ThreadHandle);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ImpersonateLoggedOnUser(HANDLE hToken)
{
    SECURITY_QUALITY_OF_SERVICE Qos;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE NewToken;
    TOKEN_TYPE Type;
    ULONG ReturnLength;
    BOOL Duplicated;
    NTSTATUS Status;

    /* Get the token type */
    Status = NtQueryInformationToken(hToken,
                                     TokenType,
                                     &Type,
                                     sizeof(TOKEN_TYPE),
                                     &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    if (Type == TokenPrimary)
    {
        /* Create a duplicate impersonation token */
        Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        Qos.ImpersonationLevel = SecurityImpersonation;
        Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Qos.EffectiveOnly = FALSE;

        ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
        ObjectAttributes.RootDirectory = NULL;
        ObjectAttributes.ObjectName = NULL;
        ObjectAttributes.Attributes = 0;
        ObjectAttributes.SecurityDescriptor = NULL;
        ObjectAttributes.SecurityQualityOfService = &Qos;

        Status = NtDuplicateToken(hToken,
                                  TOKEN_IMPERSONATE | TOKEN_QUERY,
                                  &ObjectAttributes,
                                  FALSE,
                                  TokenImpersonation,
                                  &NewToken);
        if (!NT_SUCCESS(Status))
        {
            ERR("NtDuplicateToken failed: Status %08x\n", Status);
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }

        Duplicated = TRUE;
    }
    else
    {
        /* User the original impersonation token */
        NewToken = hToken;
        Duplicated = FALSE;
    }

    /* Impersonate the the current thread */
    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    &NewToken,
                                    sizeof(HANDLE));

    if (Duplicated != FALSE)
    {
        NtClose(NewToken);
    }

    if (!NT_SUCCESS(Status))
    {
        ERR("NtSetInformationThread failed: Status %08x\n", Status);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}
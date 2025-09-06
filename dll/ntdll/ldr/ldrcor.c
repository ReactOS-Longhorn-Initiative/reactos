/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrcor.c
 * PURPOSE:         Minimal CLR (.NET) image handling hooks for the loader
 * PROGRAMMERS:     ReactOS Team
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/* Optional MSCOREE handoff */
static PVOID g_MscoreeHandle;
static PVOID g_EncodedCorValidateImage;
static PVOID g_EncodedCorDllMain;
static PVOID g_EncodedCorImageUnloading;
static BOOLEAN g_MscoreeAttempted;

typedef HRESULT (WINAPI *PFN_CorValidateImage)(PVOID* ImageBase, LPCWSTR ImageName);
typedef BOOL (WINAPI *PFN_CorDllMain)(HINSTANCE, DWORD, LPVOID);
typedef VOID (WINAPI *PFN_CorImageUnloading)(PVOID);

/* hostfxr */
typedef int (WINAPI *PFN_hostfxr_main)(int argc, const wchar_t* argv[]);
typedef int (WINAPI *PFN_hostfxr_main_startupinfo)(int argc,
                                                   const wchar_t* argv[],
                                                   const wchar_t* host_path,
                                                   const wchar_t* dotnet_root,
                                                   const wchar_t* app_path);
static PVOID g_HostFxrHandle;
static PVOID g_EncodedHostFxrMain;
static PVOID g_EncodedHostFxrMainSI;

static NTSTATUS LdrpCorEnsureMscoreeLoadedInternal(VOID)
{
    if (g_MscoreeAttempted)
        return g_MscoreeHandle ? STATUS_SUCCESS : STATUS_DLL_NOT_FOUND;

    g_MscoreeAttempted = TRUE;

    UNICODE_STRING Mscoree;
    ANSI_STRING Name;
    PVOID Base = NULL;
    NTSTATUS Status;

    RtlInitUnicodeString(&Mscoree, L"mscoree.dll");
    Status = LdrLoadDll(NULL, NULL, &Mscoree, &Base);
    if (!NT_SUCCESS(Status))
        return Status;

    g_MscoreeHandle = Base;

    /* Resolve optional exports */
    RtlInitAnsiString(&Name, "_CorValidateImage");
    if (NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &g_EncodedCorValidateImage)))
        g_EncodedCorValidateImage = RtlEncodeSystemPointer(g_EncodedCorValidateImage);
    else
        g_EncodedCorValidateImage = NULL;

    RtlInitAnsiString(&Name, "_CorDllMain");
    if (NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &g_EncodedCorDllMain)))
        g_EncodedCorDllMain = RtlEncodeSystemPointer(g_EncodedCorDllMain);
    else
        g_EncodedCorDllMain = NULL;

    RtlInitAnsiString(&Name, "_CorImageUnloading");
    if (NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &g_EncodedCorImageUnloading)))
        g_EncodedCorImageUnloading = RtlEncodeSystemPointer(g_EncodedCorImageUnloading);
    else
        g_EncodedCorImageUnloading = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrpCorEnsureMscoreeLoaded(VOID)
{
    return LdrpCorEnsureMscoreeLoadedInternal();
}

PVOID
NTAPI
LdrpCorGetCorDllMain(VOID)
{
    if (!g_MscoreeHandle || !g_EncodedCorDllMain)
        return NULL;
    return RtlDecodeSystemPointer(g_EncodedCorDllMain);
}

BOOLEAN
NTAPI
LdrpCorTryValidateViaMscoree(IN OUT PVOID* ImageBase, IN LPCWSTR FileName, OUT NTSTATUS* StatusOptional)
{
    NTSTATUS LoadStatus = LdrpCorEnsureMscoreeLoadedInternal();
    if (!NT_SUCCESS(LoadStatus) || !g_EncodedCorValidateImage)
        return FALSE;

    PFN_CorValidateImage pfn = (PFN_CorValidateImage)RtlDecodeSystemPointer(g_EncodedCorValidateImage);
    HRESULT hr = pfn(ImageBase, FileName);
    if (StatusOptional)
        *StatusOptional = SUCCEEDED(hr) ? STATUS_SUCCESS : (NTSTATUS)hr;
    return TRUE;
}

static NTSTATUS LdrpLoadHostFxr(VOID)
{
    if (g_HostFxrHandle)
        return STATUS_SUCCESS;

    UNICODE_STRING HostFxr;
    ANSI_STRING Name;
    PVOID Base = NULL;
    NTSTATUS Status;

    RtlInitUnicodeString(&HostFxr, L"hostfxr.dll");
    Status = LdrLoadDll(NULL, NULL, &HostFxr, &Base);
    if (!NT_SUCCESS(Status))
        return Status;

    g_HostFxrHandle = Base;

    RtlInitAnsiString(&Name, "hostfxr_main_startupinfo");
    if (NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &g_EncodedHostFxrMainSI)))
        g_EncodedHostFxrMainSI = RtlEncodeSystemPointer(g_EncodedHostFxrMainSI);
    else
        g_EncodedHostFxrMainSI = NULL;

    RtlInitAnsiString(&Name, "hostfxr_main");
    if (NT_SUCCESS(LdrGetProcedureAddress(Base, &Name, 0, &g_EncodedHostFxrMain)))
        g_EncodedHostFxrMain = RtlEncodeSystemPointer(g_EncodedHostFxrMain);
    else
        g_EncodedHostFxrMain = NULL;

    return STATUS_SUCCESS;
}

static int LdrpHostFxrInvoke(PUNICODE_STRING ImagePathName, PUNICODE_STRING CommandLine)
{
    NTSTATUS Status = LdrpLoadHostFxr();
    if (!NT_SUCCESS(Status))
        return Status;

    /* Build minimal argv: [exe] */
    const wchar_t* argv_local[1];
    argv_local[0] = ImagePathName ? ImagePathName->Buffer : L"";

    if (g_EncodedHostFxrMainSI)
    {
        PFN_hostfxr_main_startupinfo pMainSI = (PFN_hostfxr_main_startupinfo)RtlDecodeSystemPointer(g_EncodedHostFxrMainSI);
        return pMainSI(1, argv_local,
                       ImagePathName ? ImagePathName->Buffer : NULL,
                       NULL,
                       ImagePathName ? ImagePathName->Buffer : NULL);
    }

    if (g_EncodedHostFxrMain)
    {
        PFN_hostfxr_main pMain = (PFN_hostfxr_main)RtlDecodeSystemPointer(g_EncodedHostFxrMain);
        return pMain(1, argv_local);
    }

    return STATUS_PROCEDURE_NOT_FOUND;
}

VOID
NTAPI
LdrpHostFxrPostInitRoutine(VOID)
{
    PPEB Peb = NtCurrentPeb();
    PUNICODE_STRING ImagePath = &Peb->ProcessParameters->ImagePathName;
    PUNICODE_STRING CmdLine = &Peb->ProcessParameters->CommandLine;

    int rc = LdrpHostFxrInvoke(ImagePath, CmdLine);

    NtTerminateProcess(NtCurrentProcess(), (NTSTATUS)rc);
}

BOOL
NTAPI
LdrpIsILOnlyImage(PVOID BaseAddress)
{
    ULONG Cor20HeaderSize;
    const PIMAGE_COR20_HEADER Cor20Header = (PIMAGE_COR20_HEADER)RtlImageDirectoryEntryToData(
        BaseAddress,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
        &Cor20HeaderSize);

    return Cor20Header &&
           Cor20HeaderSize >= sizeof(IMAGE_COR20_HEADER) &&
           (Cor20Header->Flags & COMIMAGE_FLAGS_ILONLY);
}

NTSTATUS
NTAPI
LdrpCorValidateImage(IN PVOID ImageBase, IN LPCWSTR FileName)
{
    /* Minimal validation aligned with ntdll_new: reject images that have TLS */
    ULONG Size;
    UNREFERENCED_PARAMETER(FileName);
    return RtlImageDirectoryEntryToData(ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &Size)
               ? STATUS_INVALID_IMAGE_FORMAT
               : STATUS_SUCCESS;
}



#pragma once


#define RESOURCE_ENUM_LN          0x0001
#define RESOURCE_ENUM_MUI         0x0002
#define RESOURCE_ENUM_MUI_SYSTEM  0x0004
#define RESOURCE_ENUM_VALIDATE    0x0008
#define PROCESSOR_ARCHITECTURE_NEUTRAL          11
#define PROCESSOR_ARCHITECTURE_ARM64            12
//ddk
#define SYMBOLIC_LINK_ALL_ACCESS        (STANDARD_RIGHTS_REQUIRED | 0x1)

//def ndk
NTSYSAPI
BOOLEAN
NTAPI
RtlGetProductInfo(
  _In_ ULONG OSMajorVersion,
  _In_ ULONG OSMinorVersion,
  _In_ ULONG SpMajorVersion,
  _In_ ULONG SpMinorVersion,
  _Out_ PULONG ReturnedProductType);
NTSYSAPI NTSTATUS  WINAPI RtlQueryActivationContextApplicationSettings(DWORD,HANDLE,const WCHAR*,const WCHAR*,WCHAR*,SIZE_T,SIZE_T*);
NTSYSAPI NTSTATUS  WINAPI LdrSetDefaultDllDirectories(ULONG);
NTSYSAPI NTSTATUS  WINAPI LdrRemoveDllDirectory(void*);


//rtl types
//
// Constant String Macro
//
#define RTL_CONSTANT_STRING(__SOURCE_STRING__)                  \
{                                                               \
    sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
    sizeof(__SOURCE_STRING__),                                  \
    (__SOURCE_STRING__)                                         \
}

//kernel32



BOOL WINAPI GetVolumeInformationByHandleW( HANDLE handle, WCHAR *label, DWORD label_len,
                                           DWORD *serial, DWORD *filename_len, DWORD *flags,
                                           WCHAR *fsname, DWORD fsname_len );
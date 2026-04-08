
#define STANDALONE
#include <apitest.h>

extern void func_mbstowcs(void);
extern void func_setjmp(void);
extern void func_wcstombs(void);

extern void func_abs(void);
extern void func_atoi(void);
extern void func_bsearch(void);
extern void func_bsearch_s(void);
extern void func_isalnum(void);
extern void func_isalpha(void);
extern void func_iscntrl(void);
extern void func_isdigit(void);
extern void func_isgraph(void);
extern void func_islower(void);
extern void func_isprint(void);
extern void func_ispunct(void);
extern void func_isspace(void);
extern void func_isupper(void);
extern void func_iswalnum(void);
extern void func_iswalpha(void);
extern void func_iswascii(void);
extern void func_iswcntrl(void);
extern void func_iswctype(void);
extern void func_iswdigit(void);
extern void func_iswgraph(void);
extern void func_iswlower(void);
extern void func_iswprint(void);
extern void func_iswspace(void);
extern void func_iswxdigit(void);
extern void func_isxdigit(void);
extern void func_memcmp(void);
extern void func_memcpy_s(void);
extern void func_memmove_s(void);
extern void func_qsort(void);
extern void func_qsort_s(void);
extern void func_sprintf(void);
extern void func_sprintf_s(void);
extern void func_sscanf(void);
extern void func_strcat(void);
extern void func_strcat_s(void);
extern void func_strcmp(void);
extern void func_strcpy(void);
extern void func_strcpy_s(void);
extern void func_strcspn(void);
extern void func_strlen(void);
extern void func_strncat(void);
extern void func_strncat_s(void);
extern void func_strncmp(void);
extern void func_strncpy(void);
extern void func_strncpy_s(void);
extern void func_strnlen(void);
extern void func_strpbrk(void);
extern void func_strspn(void);
extern void func_strtok_s(void);
extern void func_strtol(void);
extern void func_strtoul(void);
extern void func_swprintf_s(void);
extern void func_tolower(void);
extern void func_toupper(void);
extern void func_towlower(void);
extern void func_towupper(void);
extern void func_vsprintf(void);
extern void func_vswprintf(void);
extern void func_vswprintf_s(void);
extern void func_wcscat(void);
extern void func_wcscat_s(void);
extern void func_wcscmp(void);
extern void func_wcscpy(void);
extern void func_wcscpy_s(void);
extern void func_wcscspn(void);
extern void func_wcslen(void);
extern void func_wcsncat(void);
extern void func_wcsncat_s(void);
extern void func_wcsncmp(void);
extern void func_wcsncpy(void);
extern void func_wcsncpy_s(void);
extern void func_wcsnlen(void);
extern void func_wcspbrk(void);
extern void func_wcsspn(void);
extern void func_wcstok_s(void);
extern void func_wcstol(void);
extern void func_wcstoul(void);
extern void func_wcstox_table(void);
extern void func_xtostr_table(void);
extern void func__abs64(void);
extern void func__atoi64(void);
extern void func__errno(void);
extern void func__i64toa(void);
extern void func__i64toa_s(void);
extern void func__i64tow(void);
extern void func__i64tow_s(void);
extern void func__itoa(void);
extern void func__itoa_s(void);
extern void func__itow(void);
extern void func__itow_s(void);
extern void func__makepath_s(void);
extern void func__memccpy(void);
extern void func__memicmp(void);
extern void func__scprintf(void);
extern void func__snprintf(void);
extern void func__snprintf_s(void);
extern void func__snscanf_s(void);
extern void func__snwprintf(void);
extern void func__snwprintf_s(void);
extern void func__snwscanf_s(void);
extern void func__splitpath(void);
extern void func__splitpath_s(void);
extern void func__stricmp(void);
extern void func__strlwr(void);
extern void func__strlwr_s(void);
extern void func__strnicmp(void);
extern void func__strnset_s(void);
extern void func__strset_s(void);
extern void func__strupr(void);
extern void func__strupr_s(void);
extern void func__swprintf(void);
extern void func__ui64toa(void);
extern void func__ui64toa_s(void);
extern void func__ui64tow(void);
extern void func__ui64tow_s(void);
extern void func__ultoa(void);
extern void func__ultoa_s(void);
extern void func__ultow(void);
extern void func__ultow_s(void);
extern void func__vscprintf(void);
extern void func__vscwprintf(void);
extern void func__vsnprintf(void);
extern void func__vsnprintf_s(void);
extern void func__vsnwprintf(void);
extern void func__vsnwprintf_s(void);
extern void func__vswprintf(void);
extern void func__wcsicmp(void);
extern void func__wcslwr(void);
extern void func__wcslwr_s(void);
extern void func__wcsnicmp(void);
extern void func__wcsnset_s(void);
extern void func__wcsset_s(void);
extern void func__wcstoi64(void);
extern void func__wcstoui64(void);
extern void func__wcsupr(void);
extern void func__wcsupr_s(void);
extern void func__wmakepath_s(void);
extern void func__wsplitpath_s(void);
extern void func__wtoi(void);
extern void func__wtoi64(void);
extern void func___isascii(void);
extern void func___iscsym(void);
extern void func___iscsymf(void);
extern void func___toascii(void);

extern void func_DllLoadNotification(void);
extern void func_implicit_tls(void);
extern void func_LdrEnumResources(void);
extern void func_LdrFindResource_U(void);
extern void func_LdrLoadDll(void);
extern void func_load_notifications(void);
extern void func_NtAcceptConnectPort(void);
extern void func_NtAccessCheck(void);
extern void func_NtAccessCheckByType(void);
extern void func_NtAccessCheckByTypeResultList(void);
extern void func_NtAdjustGroupsToken(void);
extern void func_NtAdjustPrivilegesToken(void);
extern void func_NtAllocateVirtualMemory(void);
extern void func_NtApphelpCacheControl(void);
extern void func_NtCompareTokens(void);
extern void func_NtContinue(void);
extern void func_NtCreateFile(void);
extern void func_NtCreateKey(void);
extern void func_NtCreateProfile(void);
extern void func_NtCreateThread(void);
extern void func_NtDeleteKey(void);
extern void func_NtDuplicateObject(void);
extern void func_NtDuplicateToken(void);
extern void func_NtFilterToken(void);
extern void func_NtFreeVirtualMemory(void);
extern void func_NtImpersonateAnonymousToken(void);
extern void func_NtLoadUnloadKey(void);
extern void func_NtMapViewOfSection(void);
extern void func_NtMutant(void);
extern void func_NtOpenKey(void);
extern void func_NtOpenProcessToken(void);
extern void func_NtOpenThreadToken(void);
extern void func_NtProtectVirtualMemory(void);
extern void func_NtQueryInformationFile(void);
extern void func_NtQueryInformationProcess(void);
extern void func_NtQueryInformationThread(void);
extern void func_NtQueryInformationToken(void);
extern void func_NtQueryKey(void);
extern void func_NtQueryObject(void);
extern void func_NtQueryOpenSubKeys(void);
extern void func_NtQuerySection(void);
extern void func_NtQuerySystemEnvironmentValue(void);
extern void func_NtQuerySystemInformation(void);
extern void func_NtQueryValueKey(void);
extern void func_NtQueryVolumeInformationFile(void);
extern void func_NtReadFile(void);
extern void func_NtSaveKey(void);
extern void func_NtSetDefaultLocale(void);
extern void func_NtSetInformationFile(void);
extern void func_NtSetInformationProcess(void);
extern void func_NtSetInformationThread(void);
extern void func_NtSetInformationToken(void);
extern void func_NtSetValueKey(void);
extern void func_NtSetVolumeInformationFile(void);
extern void func_NtStartProfile(void);
extern void func_NtSystemInformation(void);
extern void func_NtUnloadDriver(void);
extern void func_NtWriteFile(void);
extern void func_RtlAllocateHeap(void);
extern void func_RtlBitmapApi(void);
extern void func_RtlCaptureContext(void);
extern void func_RtlComputePrivatizedDllName_U(void);
extern void func_RtlCopyMappedMemory(void);
extern void func_RtlCriticalSection(void);
extern void func_RtlDebugInformation(void);
extern void func_RtlDeleteAce(void);
extern void func_RtlDetermineDosPathNameType(void);
extern void func_RtlDosApplyFileIsolationRedirection_Ustr(void);
extern void func_RtlDoesFileExists(void);
extern void func_RtlDosPathNameToNtPathName_U(void);
extern void func_RtlDosSearchPath_U(void);
extern void func_RtlDosSearchPath_Ustr(void);
extern void func_RtlFirstFreeAce(void);
extern void func_RtlGenerate8dot3Name(void);
extern void func_RtlGetFullPathName_U(void);
extern void func_RtlGetFullPathName_Ustr(void);
extern void func_RtlGetFullPathName_UstrEx(void);
extern void func_RtlGetLengthWithoutLastFullDosOrNtPathElement(void);
extern void func_RtlGetLengthWithoutTrailingPathSeperators(void);
extern void func_RtlGetLongestNtPathLength(void);
extern void func_RtlGetNtProductType(void);
extern void func_RtlGetProcessHeaps(void);
extern void func_RtlGetProductInfo(void);
extern void func_RtlGetUnloadEventTrace(void);
extern void func_RtlHandle(void);
extern void func_RtlImageDirectoryEntryToData(void);
extern void func_RtlImageRvaToVa(void);
extern void func_RtlIntSafe(void);
extern void func_RtlIsNameInExpression(void);
extern void func_RtlIsNameLegalDOS8Dot3(void);
extern void func_RtlLocale(void);
extern void func_RtlMemoryStream(void);
extern void func_RtlMultipleAllocateHeap(void);
extern void func_RtlNtPathNameToDosPathName(void);
extern void func_RtlpApplyLengthFunction(void);
extern void func_RtlpEnsureBufferSize(void);
extern void func_RtlQueryEnvironmentVariable(void);
extern void func_RtlQueryEnvironmentVariable_U(void);
extern void func_RtlQueryTimeZoneInformation(void);
extern void func_RtlReAllocateHeap(void);
extern void func_RtlRemovePrivileges(void);
extern void func_RtlUnhandledExceptionFilter(void);
extern void func_RtlUnicodeStringToAnsiString(void);
extern void func_RtlUnicodeStringToCountedOemString(void);
extern void func_RtlUnicodeToOemN(void);
extern void func_RtlUnwind(void);
extern void func_RtlUpcaseUnicodeStringToCountedOemString(void);
extern void func_RtlValidateUnicodeString(void);
extern void func_RtlVirtualUnwind(void);
extern void func_RtlxUnicodeStringToAnsiSize(void);
extern void func_RtlxUnicodeStringToOemSize(void);
extern void func_StackOverflow(void);
extern void func_TimerResolution(void);
extern void func_UserModeException(void);
extern void func_WakeAddress(void);

const struct test winetest_testlist[] =
{
    // CRT tests from ../crt
    { "mbstowcs", func_mbstowcs },
    { "setjmp", func_setjmp },
    { "wcstombs", func_wcstombs },

    { "abs", func_abs },
    { "atoi", func_atoi },
    { "bsearch", func_bsearch },
    { "bsearch_s", func_bsearch_s },
    { "isalnum", func_isalnum },
    { "isalpha", func_isalpha },
    { "iscntrl", func_iscntrl },
    { "isdigit", func_isdigit },
    { "isgraph", func_isgraph },
    { "islower", func_islower },
    { "isprint", func_isprint },
    { "ispunct", func_ispunct },
    { "isspace", func_isspace },
    { "isupper", func_isupper },
    { "iswalnum", func_iswalnum },
    { "iswalpha", func_iswalpha },
    { "iswascii", func_iswascii },
    { "iswcntrl", func_iswcntrl },
    { "iswctype", func_iswctype },
    { "iswdigit", func_iswdigit },
    { "iswgraph", func_iswgraph },
    { "iswlower", func_iswlower },
    { "iswprint", func_iswprint },
    { "iswspace", func_iswspace },
    { "iswxdigit", func_iswxdigit },
    { "isxdigit", func_isxdigit },
    { "memcmp", func_memcmp },
    { "memcpy_s", func_memcpy_s },
    { "memmove_s", func_memmove_s },
    { "qsort", func_qsort },
    { "qsort_s", func_qsort_s },
    { "sprintf", func_sprintf },
    { "sprintf_s", func_sprintf_s },
    { "sscanf", func_sscanf },
    { "strcat", func_strcat },
    { "strcat_s", func_strcat_s },
    { "strcmp", func_strcmp },
    { "strcpy", func_strcpy },
    { "strcpy_s", func_strcpy_s },
    { "strcspn", func_strcspn },
    { "strlen", func_strlen },
    { "strncat", func_strncat },
    { "strncat_s", func_strncat_s },
    { "strncmp", func_strncmp },
    { "strncpy", func_strncpy },
    { "strncpy_s", func_strncpy_s },
    { "strnlen", func_strnlen },
    { "strpbrk", func_strpbrk },
    { "strspn", func_strspn },
    { "strtok_s", func_strtok_s },
    { "strtol", func_strtol },
    { "strtoul", func_strtoul },
    { "swprintf_s", func_swprintf_s },
    { "tolower", func_tolower },
    { "toupper", func_toupper },
    { "towlower", func_towlower },
    { "towupper", func_towupper },
    //{ "vsprintf", func_vsprintf },
    //{ "vswprintf", func_vswprintf },
    //{ "vswprintf_s", func_vswprintf_s },
    { "wcscat", func_wcscat },
    { "wcscat_s", func_wcscat_s },
    { "wcscmp", func_wcscmp },
    { "wcscpy", func_wcscpy },
    { "wcscpy_s", func_wcscpy_s },
    { "wcscspn", func_wcscspn },
    { "wcslen", func_wcslen },
    { "wcsncat", func_wcsncat },
    { "wcsncat_s", func_wcsncat_s },
    { "wcsncmp", func_wcsncmp },
    { "wcsncpy", func_wcsncpy },
    { "wcsncpy_s", func_wcsncpy_s },
    { "wcsnlen", func_wcsnlen },
    { "wcspbrk", func_wcspbrk },
    { "wcsspn", func_wcsspn },
    { "wcstok_s", func_wcstok_s },
    { "wcstol", func_wcstol },
    { "wcstoul", func_wcstoul },
    { "_abs64", func__abs64 },
    { "_atoi64", func__atoi64 },
    { "_errno", func__errno },
    { "_i64toa", func__i64toa },
    { "_i64toa_s", func__i64toa_s },
    { "_i64tow", func__i64tow },
    { "_i64tow_s", func__i64tow_s },
    { "_itoa", func__itoa },
    { "_itoa_s", func__itoa_s },
    { "_itow", func__itow },
    { "_itow_s", func__itow_s },
    { "_makepath_s", func__makepath_s },
    { "_memccpy", func__memccpy },
    { "_memicmp", func__memicmp },
    { "_scprintf", func__scprintf },
    { "_snprintf", func__snprintf },
    { "_snprintf_s", func__snprintf_s },
    { "_snscanf_s", func__snscanf_s },
    { "_snwprintf", func__snwprintf },
    { "_snwprintf_s", func__snwprintf_s },
    { "_snwscanf_s", func__snwscanf_s },
    { "_splitpath", func__splitpath },
    { "_splitpath_s", func__splitpath_s },
    { "_stricmp", func__stricmp },
    { "_strlwr", func__strlwr },
    { "_strlwr_s", func__strlwr_s },
    { "_strnicmp", func__strnicmp },
    { "_strnset_s", func__strnset_s },
    { "_strset_s", func__strset_s },
    { "_strupr", func__strupr },
    { "_strupr_s", func__strupr_s },
    { "_swprintf", func__swprintf },
    { "_ui64toa", func__ui64toa },
    { "_ui64toa_s", func__ui64toa_s },
    { "_ui64tow", func__ui64tow },
    { "_ui64tow_s", func__ui64tow_s },
    { "_ultoa", func__ultoa },
    { "_ultoa_s", func__ultoa_s },
    { "_ultow", func__ultow },
    { "_ultow_s", func__ultow_s },
    //{ "_vscprintf", func__vscprintf },
    { "_vscwprintf", func__vscwprintf },
    { "_vsnprintf", func__vsnprintf },
    //{ "_vsnprintf_s", func__vsnprintf_s },
    { "_vsnwprintf", func__vsnwprintf },
    //{ "_vsnwprintf_s", func__vsnwprintf_s },
    //{ "_vswprintf", func__vswprintf },
    { "_wcsicmp", func__wcsicmp },
    { "_wcslwr", func__wcslwr },
    { "_wcslwr_s", func__wcslwr_s },
    { "_wcsnicmp", func__wcsnicmp },
    { "_wcsnset_s", func__wcsnset_s },
    { "_wcsset_s", func__wcsset_s },
    { "_wcstoi64", func__wcstoi64 },
    { "_wcstoui64", func__wcstoui64 },
    { "_wcsupr", func__wcsupr },
    { "_wcsupr_s", func__wcsupr_s },
    { "_wmakepath_s", func__wmakepath_s },
    { "_wsplitpath_s", func__wsplitpath_s },
    { "_wtoi", func__wtoi },
    { "_wtoi64", func__wtoi64 },
    { "__isascii", func___isascii },
    { "__iscsym", func___iscsym },
    { "__iscsymf", func___iscsymf },
    { "__toascii", func___toascii },

    { "DllLoadNotification",            func_DllLoadNotification },
    { "implicit_tls",                   func_implicit_tls },
    { "LdrEnumResources",               func_LdrEnumResources },
    { "LdrFindResource_U",              func_LdrFindResource_U },
    { "LdrLoadDll",                     func_LdrLoadDll },
    { "load_notifications",             func_load_notifications },
    { "NtAcceptConnectPort",            func_NtAcceptConnectPort },
    { "NtAccessCheck",                  func_NtAccessCheck },
    { "NtAccessCheckByType",            func_NtAccessCheckByType },
    { "NtAccessCheckByTypeResultList",  func_NtAccessCheckByTypeResultList },
    { "NtAdjustGroupsToken",            func_NtAdjustGroupsToken },
    { "NtAdjustPrivilegesToken",        func_NtAdjustPrivilegesToken },
    { "NtAllocateVirtualMemory",        func_NtAllocateVirtualMemory },
    { "NtApphelpCacheControl",          func_NtApphelpCacheControl },
    { "NtCompareTokens",                func_NtCompareTokens },
    { "NtContinue",                     func_NtContinue },
    { "NtCreateFile",                   func_NtCreateFile },
    { "NtCreateKey",                    func_NtCreateKey },
    { "NtCreateProfile",                func_NtCreateProfile },
    { "NtCreateThread",                 func_NtCreateThread },
    { "NtDeleteKey",                    func_NtDeleteKey },
    { "NtDuplicateObject",              func_NtDuplicateObject },
    { "NtDuplicateToken",               func_NtDuplicateToken },
    { "NtFilterToken",                  func_NtFilterToken },
    { "NtFreeVirtualMemory",            func_NtFreeVirtualMemory },
    { "NtImpersonateAnonymousToken",    func_NtImpersonateAnonymousToken },
    { "NtLoadUnloadKey",                func_NtLoadUnloadKey },
    { "NtMapViewOfSection",             func_NtMapViewOfSection },
    { "NtMutant",                       func_NtMutant },
    { "NtOpenKey",                      func_NtOpenKey },
    { "NtOpenProcessToken",             func_NtOpenProcessToken },
    { "NtOpenThreadToken",              func_NtOpenThreadToken },
    { "NtProtectVirtualMemory",         func_NtProtectVirtualMemory },
    { "NtQueryInformationFile",         func_NtQueryInformationFile },
    { "NtQueryInformationProcess",      func_NtQueryInformationProcess },
    { "NtQueryInformationThread",       func_NtQueryInformationThread },
    { "NtQueryInformationToken",        func_NtQueryInformationToken },
    { "NtQueryKey",                     func_NtQueryKey },
    { "NtQueryObject",                  func_NtQueryObject },
    { "NtQueryOpenSubKeys",             func_NtQueryOpenSubKeys },
    { "NtQuerySection",                 func_NtQuerySection },
    { "NtQuerySystemEnvironmentValue",  func_NtQuerySystemEnvironmentValue },
    { "NtQuerySystemInformation",       func_NtQuerySystemInformation },
    { "NtQueryValueKey",                func_NtQueryValueKey },
    { "NtQueryVolumeInformationFile",   func_NtQueryVolumeInformationFile },
    { "NtReadFile",                     func_NtReadFile },
    { "NtSaveKey",                      func_NtSaveKey},
    { "NtSetDefaultLocale",             func_NtSetDefaultLocale },
    { "NtSetInformationFile",           func_NtSetInformationFile },
    { "NtSetInformationProcess",        func_NtSetInformationProcess },
    { "NtSetInformationThread",         func_NtSetInformationThread },
    { "NtSetInformationToken",          func_NtSetInformationToken },
    { "NtSetValueKey",                  func_NtSetValueKey},
    { "NtSetVolumeInformationFile",     func_NtSetVolumeInformationFile },
    { "NtStartProfile",                 func_NtStartProfile },
    { "NtSystemInformation",            func_NtSystemInformation },
    { "NtUnloadDriver",                 func_NtUnloadDriver },
    { "NtWriteFile",                    func_NtWriteFile },
    { "RtlAllocateHeap",                func_RtlAllocateHeap },
    { "RtlBitmapApi",                   func_RtlBitmapApi },
    { "RtlComputePrivatizedDllName_U",  func_RtlComputePrivatizedDllName_U },
    { "RtlCopyMappedMemory",            func_RtlCopyMappedMemory },
    { "RtlCriticalSection",             func_RtlCriticalSection },
    { "RtlDebugInformation",            func_RtlDebugInformation },
    { "RtlDeleteAce",                   func_RtlDeleteAce },
    { "RtlDetermineDosPathNameType",    func_RtlDetermineDosPathNameType },
    { "RtlDosApplyFileIsolationRedirection_Ustr", func_RtlDosApplyFileIsolationRedirection_Ustr },
    { "RtlDoesFileExists",              func_RtlDoesFileExists },
    { "RtlDosPathNameToNtPathName_U",   func_RtlDosPathNameToNtPathName_U },
    { "RtlDosSearchPath_U",             func_RtlDosSearchPath_U },
    { "RtlDosSearchPath_Ustr",          func_RtlDosSearchPath_Ustr },
    { "RtlFirstFreeAce",                func_RtlFirstFreeAce },
    { "RtlGenerate8dot3Name",           func_RtlGenerate8dot3Name },
    { "RtlGetFullPathName_U",           func_RtlGetFullPathName_U },
    { "RtlGetFullPathName_Ustr",        func_RtlGetFullPathName_Ustr },
    { "RtlGetFullPathName_UstrEx",      func_RtlGetFullPathName_UstrEx },
    { "RtlGetLengthWithoutLastFullDosOrNtPathElement", func_RtlGetLengthWithoutLastFullDosOrNtPathElement },
    { "RtlGetLengthWithoutTrailingPathSeperators", func_RtlGetLengthWithoutTrailingPathSeperators },
    { "RtlGetLongestNtPathLength",      func_RtlGetLongestNtPathLength },
    { "RtlGetNtProductType",            func_RtlGetNtProductType },
    { "RtlGetProcessHeaps",             func_RtlGetProcessHeaps },
    { "RtlGetProductInfo",              func_RtlGetProductInfo },
    { "RtlGetUnloadEventTrace",         func_RtlGetUnloadEventTrace },
    { "RtlHandle",                      func_RtlHandle },
    { "RtlImageDirectoryEntryToData",   func_RtlImageDirectoryEntryToData },
    { "RtlImageRvaToVa",                func_RtlImageRvaToVa },
    { "RtlIntSafe",                     func_RtlIntSafe },
    { "RtlIsNameInExpression",          func_RtlIsNameInExpression},
    { "RtlIsNameLegalDOS8Dot3",         func_RtlIsNameLegalDOS8Dot3 },
    { "RtlLocale",                      func_RtlLocale },
    { "RtlMemoryStream",                func_RtlMemoryStream },
    { "RtlMultipleAllocateHeap",        func_RtlMultipleAllocateHeap },
    { "RtlNtPathNameToDosPathName",     func_RtlNtPathNameToDosPathName },
    { "RtlpApplyLengthFunction",        func_RtlpApplyLengthFunction },
    { "RtlpEnsureBufferSize",           func_RtlpEnsureBufferSize },
    { "RtlQueryEnvironmentVariable",    func_RtlQueryEnvironmentVariable },
    { "RtlQueryEnvironmentVariable_U",  func_RtlQueryEnvironmentVariable_U },
    { "RtlQueryTimeZoneInformation",    func_RtlQueryTimeZoneInformation },
    { "RtlReAllocateHeap",              func_RtlReAllocateHeap },
    { "RtlRemovePrivileges",            func_RtlRemovePrivileges },
    { "RtlUnhandledExceptionFilter",    func_RtlUnhandledExceptionFilter },
    { "RtlUnicodeStringToAnsiSize",     func_RtlxUnicodeStringToAnsiSize }, /* For some reason, starting test name with Rtlx hides it */
    { "RtlUnicodeStringToAnsiString",   func_RtlUnicodeStringToAnsiString },
    { "RtlUnicodeStringToCountedOemString", func_RtlUnicodeStringToCountedOemString },
    { "RtlUnicodeStringToOemSize",      func_RtlxUnicodeStringToOemSize },
    { "RtlUnicodeToOemN",               func_RtlUnicodeToOemN },
    { "RtlUpcaseUnicodeStringToCountedOemString", func_RtlUpcaseUnicodeStringToCountedOemString },
    { "RtlValidateUnicodeString",       func_RtlValidateUnicodeString },
    { "RtlWaitOnAddress",               func_WakeAddress },
    { "StackOverflow",                  func_StackOverflow },
    { "TimerResolution",                func_TimerResolution },
    { "UserModeException",              func_UserModeException },
#ifdef _M_IX86
    { "RtlUnwind",                      func_RtlUnwind },
#endif
#ifdef _M_AMD64
    { "RtlCaptureContext",              func_RtlCaptureContext },
    { "RtlVirtualUnwind",               func_RtlVirtualUnwind },
#endif

    { 0, 0 }
};

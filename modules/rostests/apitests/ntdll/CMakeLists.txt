
add_subdirectory(implicit_tls)
add_subdirectory(load_notifications)
add_subdirectory(empty_dll)

include_directories($<TARGET_FILE_DIR:implicit_tls>)
include_directories($<TARGET_FILE_DIR:load_notifications>)
include_directories($<TARGET_FILE_DIR:empty_dll>)
include_directories(${REACTOS_SOURCE_DIR}/ntoskrnl/include)
spec2def(ntdll_apitest.exe ntdll_apitest.spec)

list(APPEND SOURCE_CRT_TESTS
    #../crt/_snprintf.c
    #../crt/_snwprintf.c
    #../crt/_stricmp.c
    #../crt/_strlwr.c
    #../crt/_strnicmp.c
    #../crt/_strupr.c
    ../crt/_vscwprintf.c
    ../crt/_vsnprintf.c
    ../crt/_vsnwprintf.c
    #../crt/_wcsicmp.c
    #../crt/_wcslwr.c
    #../crt/_wcsnicmp.c
    #../crt/_wcsupr.c
    ../crt/mbstowcs.c
    ../crt/setjmp.c
    #../crt/sprintf.c
    #../crt/strcpy.c
    #../crt/strlen.c
    #../crt/strtoul.c
    ../crt/wcstombs.c
    #../crt/wcstoul.c
)

list(APPEND SOURCE_CRT_TESTS
    crt/abs.c
    crt/atoi.c
    crt/bsearch.c
    crt/bsearch_s.c
    crt/isalnum.c
    crt/isalpha.c
    crt/iscntrl.c
    crt/isdigit.c
    crt/isgraph.c
    crt/islower.c
    crt/isprint.c
    crt/ispunct.c
    crt/isspace.c
    crt/isupper.c
    crt/iswalnum.c
    crt/iswalpha.c
    crt/iswascii.c
    crt/iswcntrl.c
    crt/iswctype.c
    crt/iswdigit.c
    crt/iswgraph.c
    crt/iswlower.c
    crt/iswprint.c
    crt/iswspace.c
    crt/iswxdigit.c
    crt/isxdigit.c
    crt/memcmp.c
    crt/memcpy_s.c
    crt/memmove_s.c
    crt/printf_formats.h
    crt/qsort.c
    crt/qsort_s.c
    crt/scanf_formats.h
    crt/sprintf.c
    crt/sprintf_s.c
    crt/sscanf.c
    crt/sscanf_s.c
    crt/strcat.c
    crt/strcat_s.c
    crt/strcmp.c
    crt/strcpy.c
    crt/strcpy_s.c
    crt/strcspn.c
    crt/strlen.c
    crt/strncat.c
    crt/strncat_s.c
    crt/strncmp.c
    crt/strncpy.c
    crt/strncpy_s.c
    crt/strnlen.c
    crt/strpbrk.c
    crt/strspn.c
    crt/strtok_s.c
    crt/strtol.c
    crt/strtoul.c
    crt/strtox_table.c
    crt/strtox_table.h
    crt/swprintf_s.c
    crt/swscanf_s.c
    crt/test.c
    #crt/test.h
    crt/tolower.c
    crt/toupper.c
    crt/towlower.c
    crt/towupper.c
    crt/vsprintf.c
    crt/vswprintf.c
    crt/vswprintf_s.c
    crt/wcscat.c
    crt/wcscat_s.c
    crt/wcscmp.c
    crt/wcscpy.c
    crt/wcscpy_s.c
    crt/wcscspn.c
    crt/wcslen.c
    crt/wcsncat.c
    crt/wcsncat_s.c
    crt/wcsncmp.c
    crt/wcsncpy.c
    crt/wcsncpy_s.c
    crt/wcsnlen.c
    crt/wcspbrk.c
    crt/wcsspn.c
    crt/wcstok_s.c
    crt/wcstol.c
    crt/wcstoul.c
    crt/wcstox_table.c
    crt/xtostr_table.c
    crt/xtostr_table.h
    crt/xtowcs_table.c
    #crt/_abs64.c
    crt/_atoi64.c
    crt/_errno.c
    crt/_i64toa.c
    crt/_i64toa_s.c
    crt/_i64tow.c
    crt/_i64tow_s.c
    crt/_itoa.c
    crt/_itoa_s.c
    crt/_itow.c
    crt/_itow_s.c
    crt/_makepath_s.c
    crt/_memccpy.c
    crt/_memicmp.c
    crt/_scprintf.c
    crt/_snprintf.c
    crt/_snprintf_s.c
    crt/_snscanf_s.c
    crt/_snwprintf.c
    crt/_snwprintf_s.c
    crt/_snwscanf_s.c
    crt/_splitpath.c
    crt/_splitpath_s.c
    crt/_stricmp.c
    crt/_strlwr.c
    crt/_strlwr_s.c
    crt/_strnicmp.c
    crt/_strnset_s.c
    crt/_strset_s.c
    crt/_strupr.c
    crt/_strupr_s.c
    crt/_swprintf.c
    crt/_ui64toa.c
    crt/_ui64toa_s.c
    crt/_ui64tow.c
    crt/_ui64tow_s.c
    crt/_ultoa.c
    crt/_ultoa_s.c
    crt/_ultow.c
    crt/_ultow_s.c
    crt/_vscprintf.c
    crt/_vscwprintf.c
    crt/_vsnprintf.c
    crt/_vsnprintf_s.c
    crt/_vsnwprintf.c
    crt/_vsnwprintf_s.c
    crt/_vswprintf.c
    crt/_wcsicmp.c
    crt/_wcslwr.c
    crt/_wcslwr_s.c
    crt/_wcsnicmp.c
    crt/_wcsnset_s.c
    crt/_wcsset_s.c
    crt/_wcstoi64.c
    crt/_wcstoui64.c
    crt/_wcsupr.c
    crt/_wcsupr_s.c
    crt/_wmakepath_s.c
    crt/_wsplitpath_s.c
    crt/_wtoi.c
    crt/_wtoi64.c
    crt/__isascii.c
    crt/__iscsym.c
    crt/__iscsymf.c
    crt/__toascii.c
)

add_library(ntdll_crt_test_lib ${SOURCE_CRT_TESTS})
target_compile_definitions(ntdll_crt_test_lib PRIVATE __STDC_WANT_SECURE_LIB__ TEST_NTDLL TEST_DLL_NAME="ntdll.dll")
target_link_libraries(ntdll_crt_test_lib ${PSEH_LIB})
add_dependencies(ntdll_crt_test_lib psdk)

list(APPEND SOURCE
    DllLoadNotification.c
    implicit_tls.c
    LdrEnumResources.c
    LdrFindResource_U.c
    LdrLoadDll.c
    load_notifications.c
    locale.c
    NtAcceptConnectPort.c
    NtAccessCheck.c
    NtAccessCheckByType.c
    NtAccessCheckByTypeResultList.c
    NtAdjustGroupsToken.c
    NtAdjustPrivilegesToken.c
    NtAllocateVirtualMemory.c
    NtApphelpCacheControl.c
    NtCompareTokens.c
    NtContinue.c
    NtCreateFile.c
    NtCreateKey.c
    NtCreateProfile.c
    NtCreateThread.c
    NtDeleteKey.c
    NtDuplicateObject.c
    NtDuplicateToken.c
    NtFilterToken.c
    NtFreeVirtualMemory.c
    NtGetCurrentProcessorNumberEx.c
    NtImpersonateAnonymousToken.c
    NtLoadUnloadKey.c
    NtMapViewOfSection.c
    NtMutant.c
    NtOpenKey.c
    NtOpenProcessToken.c
    NtOpenThreadToken.c
    NtProtectVirtualMemory.c
    NtQueryInformationFile.c
    NtQueryInformationProcess.c
    NtQueryInformationThread.c
    NtQueryInformationToken.c
    NtQueryKey.c
    NtQueryObject.c
    NtQueryOpenSubKeys.c
    NtQuerySection.c
    NtQuerySystemEnvironmentValue.c
    NtQuerySystemInformation.c
    NtQueryValueKey.c
    NtQueryVolumeInformationFile.c
    NtReadFile.c
    NtSaveKey.c
    NtSetDefaultLocale.c
    NtSetInformationFile.c
    NtSetInformationProcess.c
    NtSetInformationThread.c
    NtSetInformationToken.c
    NtSetValueKey.c
    NtSetVolumeInformationFile.c
    NtStartProfile.c
    NtUnloadDriver.c
    NtWriteFile.c
    probelib.c
    RtlAllocateHeap.c
    RtlBitmap.c
    RtlComputePrivatizedDllName_U.c
    RtlCopyMappedMemory.c
    RtlCriticalSection.c
    RtlDebugInformation.c
    RtlDeleteAce.c
    RtlDetermineDosPathNameType.c
    RtlDoesFileExists.c
    RtlDosApplyFileIsolationRedirection_Ustr.c
    RtlDosPathNameToNtPathName_U.c
    RtlDosSearchPath_U.c
    RtlDosSearchPath_Ustr.c
    RtlFirstFreeAce.c
    RtlFls.c
    RtlGenerate8dot3Name.c
    RtlGetFullPathName_U.c
    RtlGetFullPathName_Ustr.c
    RtlGetFullPathName_UstrEx.c
    RtlGetLengthWithoutLastFullDosOrNtPathElement.c
    RtlGetLengthWithoutTrailingPathSeperators.c
    RtlGetLongestNtPathLength.c
    RtlGetNtProductType.c
    RtlGetProcessHeaps.c
    RtlGetProductInfo.c
    RtlGetUnloadEventTrace.c
    RtlHandle.c
    RtlImageDirectoryEntryToData.c
    RtlImageRvaToVa.c
    RtlInitializeContext.c
    RtlIsNameInExpression.c
    RtlInitializeNtUserPfn.c
    RtlIsNameLegalDOS8Dot3.c
    RtlLocale.c
    RtlMemoryStream.c
    RtlMultipleAllocateHeap.c
    RtlNtPathNameToDosPathName.c
    RtlpApplyLengthFunction.c
    RtlpEnsureBufferSize.c
    RtlQueryEnvironmentVariable.c
    RtlQueryEnvironmentVariable_U.c
    RtlQueryTimeZoneInfo.c
    RtlReAllocateHeap.c
    RtlRemovePrivileges.c
    RtlRetrieveNtUserPfn.c
    RtlUnhandledExceptionFilter.c
    RtlUnicodeStringToAnsiString.c
    RtlUnicodeStringToCountedOemString.c
    RtlUnicodeToOemN.c
    RtlUpcaseUnicodeStringToCountedOemString.c
    RtlValidateUnicodeString.c
    RtlWaitOnAddress.c
    RtlxUnicodeStringToAnsiSize.c
    RtlxUnicodeStringToOemSize.c
    StackOverflow.c
    SystemInfo.c
    UserModeException.c
    Timer.c
    precomp.h)

if(ARCH STREQUAL "i386")
    add_asm_files(ntdll_apitest_asm
        ../crt/i386/setjmp_helper.s
        i386/NtContinue.S
        i386/NtCreateThread.S
    )
elseif(ARCH STREQUAL "amd64")
    add_asm_files(ntdll_apitest_asm
        ../crt/amd64/setjmp_helper.s
        amd64/NtContinue.S
        amd64/NtCreateThread.S
    )
endif()

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_rc_deps(testdata.rc ${CMAKE_CURRENT_BINARY_DIR}/implicit_tls/implicit_tls.dll)
add_rc_deps(testdata.rc ${CMAKE_CURRENT_BINARY_DIR}/load_notifications/load_notifications.dll)
add_rc_deps(testdata.rc ${CMAKE_CURRENT_BINARY_DIR}/empty_dll/empty_dll.dll)

add_executable(ntdll_apitest
    ${SOURCE}
    ${ntdll_apitest_asm}
    ${PCH_SKIP_SOURCE}
    testdata.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_apitest.def)

set_target_properties(ntdll_apitest
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

target_link_libraries(ntdll_apitest ntdll_crt_test_lib rtl_test_lib wine uuid ${PSEH_LIB})
set_module_type(ntdll_apitest win32cui)
add_delay_importlibs(ntdll_apitest advapi32)
add_importlibs(ntdll_apitest msvcrt kernel32 ntdll)
add_pch(ntdll_apitest precomp.h "${PCH_SKIP_SOURCE}")
add_dependencies(ntdll_apitest load_notifications empty_dll implicit_tls)

if(NOT MSVC)
    set_source_files_properties(
        RtlGetFullPathName_UstrEx.c
        RtlLocale.c
        PROPERTIES COMPILE_OPTIONS "-Wno-format")
endif()

add_rostests_file(TARGET ntdll_apitest)
add_rostests_file(TARGET ntdll_apitest SUBDIR testdata)
# These are empty files
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/ntdll_apitest.exe.local" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/shell32.dll" SUBDIR testdata)
# add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/test.dll" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/ntdlltest.manifest" SUBDIR testdata)
add_rostests_file(FILE "${CMAKE_CURRENT_SOURCE_DIR}/fil1.txt" SUBDIR testdata)

# Convenience target for running from IDE
if(MSVC_IDE)
    set(NTDLL_APITEST_DATADIR $<TARGET_FILE_DIR:ntdll_apitest>/testdata/)
    add_custom_command(TARGET ntdll_apitest
                       POST_BUILD
                       COMMAND "${CMAKE_COMMAND}" -E make_directory ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:ntdll_apitest> ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy ntdll_apitest.exe.local ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy shell32.dll ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy test.dll ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy ntdlltest.manifest ${NTDLL_APITEST_DATADIR}
                       COMMAND "${CMAKE_COMMAND}" -E copy fil1.txt ${NTDLL_APITEST_DATADIR}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_definitions(-D_WIN32_WINNT=0xA01 -DWINVER=0xA01)

add_definitions(
    -D__WINESRC__
    -D_WINE
    -Dwcsnicmp=_wcsnicmp
    -Dwcsicmp=_wcsicmp
    -Dstrnicmp=_strnicmp
    -D_KERNEL32_
    -D_KERNELBASE_)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Silence GCC/Clang warnings
    add_compile_options(
        -Wno-unused-function
        -Wno-unknown-pragmas
        -Wno-unused-variable
        -Wno-unused-value
        -Wno-unused-function
    )
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>)
endif()

#list(APPEND SOURCE_PATH
#    path.c
#    )
#
#add_library(kernelbase_path ${SOURCE_PATH})
#target_link_libraries(kernelbase_path ${PSEH_LIB})
#add_dependencies(kernelbase_path psdk)

if(NOT MSVC)
    #target_compile_options(kernelbase_path PRIVATE -Wno-unused-variable)
    #target_compile_options(kernelbase_path PRIVATE -Wno-unused-function)
endif()

# Every file in dlls/kernelbase now *compiles* against our headers. The ones
# listed below also link; the rest are held back only by ntdll exports we do
# not have yet. Move a file up as soon as its blockers land.
list(APPEND SOURCE_STATIC
    console.c
    debug.c
    locale.c
    path.c
    security.c
    string.c
    version.c
    volume.c
    )

# Compiles, but does not link yet. Blocking ntdll exports, per file:
#
#   file.c      RtlGetSearchPath, RtlReleasePath, RtlGetSystemTimePrecise
#   loader.c    LdrSetDefaultDllDirectories, LdrAddDllDirectory,
#               LdrRemoveDllDirectory, LdrGetDllPath, LdrGetDllFullName,
#               ApiSetQueryApiSetPresenceEx, RtlReleasePath;
#               RtlCreateActivationContext is declared @24 in ntdll.spec but
#               Wine calls the @8 (NT6) form
#   memory.c    NtAllocateVirtualMemoryEx, NtMapViewOfSectionEx,
#               NtUnmapViewOfSectionEx, NtSetInformationVirtualMemory,
#               RtlCopyContext, and the extended-context/XState family
#               (RtlGetExtendedContextLength2, RtlInitializeExtendedContext2,
#               RtlLocateExtendedFeature, RtlGetExtendedFeaturesMask,
#               RtlSetExtendedFeaturesMask, RtlGetEnabledExtendedFeatures)
#   process.c   NtCompareObjects, RtlGetExePath, RtlReleasePath
#   registry.c  NtOpenKeyEx
#   sync.c      RtlQueryUnbiasedInterruptTime
#   thread.c    RtlFlsGetValue, RtlFlsSetValue; RtlFlsAlloc is declared @16 in
#               ntdll.spec but Wine calls the @8 (NT6) form
#   main.c      needs init_locale() from locale.c (still #ifndef __REACTOS__)
#               and IID_IUnknown (link against uuid)
#
#list(APPEND SOURCE_STATIC
#    file.c
#    loader.c
#    main.c
#    memory.c
#    process.c
#    registry.c
#    sync.c
#    thread.c
#    )

add_library(kernelbase_static ${SOURCE_STATIC})
target_compile_definitions(kernelbase_static PRIVATE WINSHLWAPI= WINPATHCCHAPI= PATHCCH_NO_DEPRECATE=)
target_link_libraries(kernelbase_static psapi_static ${PSEH_LIB})
add_dependencies(kernelbase_static psdk)

if(MSVC)
    target_compile_options(kernelbase_static PRIVATE
        /wd4267 # warning C4267: '=': conversion from 'size_t' to 'unsigned int', possible loss of data
        /wd4146 # warning C4146: unary minus operator applied to unsigned type, result still unsigned
    )
else()
    target_compile_options(kernelbase_static PRIVATE -Wno-unused-variable)
    target_compile_options(kernelbase_static PRIVATE -Wno-unused-function)
endif()


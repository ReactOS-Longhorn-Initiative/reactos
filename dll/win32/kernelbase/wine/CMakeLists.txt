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

# Every file in dlls/kernelbase now *compiles* against our headers. The eight
# listed below also link; the eight in the comment block that follows are held
# back only by ntdll exports we do not have yet. Move a file up as soon as its
# blockers land.
list(APPEND SOURCE_STATIC
    console.c
    debug.c
    file.c
    loader.c
    locale.c
    main.c
    memory.c
    path.c
    process.c
    registry.c
    security.c
    string.c
    sync.c
    thread.c
    version.c
    volume.c
    )

# Held back. Each list below is exact: it was produced by putting all sixteen
# files in the archive and reading the linker's unresolved set, not by guessing.
# Decorations are the __stdcall arities Wine calls with.
#
#   sync.c      RtlQueryUnbiasedInterruptTime@4
#               Cheapest one left. KUSER_SHARED_DATA already carries
#               InterruptTimeBias (ndk/ketypes.h), so this is a few lines.
#
#   thread.c    RtlCreateUserStack@24, RtlFreeUserStack@4
#               Commented out in ntdll.spec (see "TODO HACK RtlCreateUserStack
#               / RtlFreeUserStack") with no implementation in sdk/lib/rtl.
#               kernel32's BaseCreateStack() in client/utils.c is the same
#               algorithm and is the obvious starting point.
#
#   registry.c  NtOpenKeyEx@16
#   process.c   NtCompareObjects@8, RtlGetExePath@8, RtlReleasePath@4
#   file.c      RtlGetSearchPath@4, RtlReleasePath@4, RtlGetSystemTimePrecise@0
#               RtlGetSystemTimePrecise is cheap (KUSER_SHARED_DATA); the two
#               path helpers are the NT6 RtlGetSearchPath/RtlReleasePath pair
#               that process.c and loader.c also want.
#
#   loader.c    The NT6 LOAD_LIBRARY_SEARCH_* loader surface, none of which
#               exists: LdrSetDefaultDllDirectories@4, LdrAddDllDirectory@8,
#               LdrRemoveDllDirectory@4, LdrGetDllPath@16, LdrGetDllFullName@8,
#               ApiSetQueryApiSetPresenceEx@12, RtlReleasePath@4. Also
#               RtlCreateActivationContext: we export the NT5 @24 form, Wine
#               calls the NT6 @8 form.
#
#   memory.c    Four NT6/NT10 syscalls with no kernel side at all:
#               NtAllocateVirtualMemoryEx@28, NtMapViewOfSectionEx@36,
#               NtUnmapViewOfSectionEx@12, NtSetInformationVirtualMemory@24.
#               The XState/extended-context family and RtlCopyContext that
#               used to block this file are all implemented now.
#
#   main.c      Not an ntdll problem: needs init_locale() from locale.c (still
#               inside the big #ifndef __REACTOS__ block, which currently
#               disables ~98% of that file) and IID_IUnknown, which means
#               adding uuid to kernelbase_ros's link libraries.
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


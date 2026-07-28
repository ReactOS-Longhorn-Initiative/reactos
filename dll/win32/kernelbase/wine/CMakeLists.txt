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

# Fifteen of the sixteen files in Wine's dlls/kernelbase now compile *and*
# link. Only main.c is held back, and not by a missing ntdll export - see the
# note below it.
list(APPEND SOURCE_STATIC
    console.c
    debug.c
    file.c
    loader.c
    locale.c
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

# main.c is the last one out. Its blockers are not ntdll exports:
#
#   * it defines its own DllMain, which collides with our DllMain.c
#   * it needs init_locale() from locale.c, which is still inside the big
#     #ifndef __REACTOS__ block that disables ~98% of that file because
#     kernel32 owns NLS today
#   * it references IID_IUnknown, so kernelbase_ros would need to link uuid
#
# Bringing it up is really the "who owns locale" decision, not a bring-up step.
#
#list(APPEND SOURCE_STATIC
#    main.c
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


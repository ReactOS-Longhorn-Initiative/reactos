
# Replace the old CRT include directory with the UCRT include directory
get_property(INCLUDE_DIRS DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
list(REMOVE_ITEM INCLUDE_DIRS "${REACTOS_SOURCE_DIR}/sdk/include/crt")
set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${INCLUDE_DIRS})
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/ucrt)

include_directories(inc)

if(DBG)
    add_compile_definitions(_DEBUG) # TODO: define this globally
endif()

add_compile_definitions(
    _CORECRT_BUILD
    _VCRT_BUILD
    _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
    _NTSYSTEM_
)

if(${ARCH} STREQUAL "i386")
    list(APPEND VCRT_SETJMP_SOURCES
        i386/longjmp.c
    )
    list(APPEND VCRT_SETJMP_ASM_SOURCES
        i386/__longjmp_nounwind.s
        i386/_seh_longjmp_unwind.s
        i386/_setjmp.s
        i386/_setjmp3.s
    )
elseif(${ARCH} STREQUAL "amd64")
    list(APPEND VCRT_SETJMP_SOURCES
        amd64/longjmp.c
    )
    list(APPEND VCRT_SETJMP_ASM_SOURCES
        amd64/__longjmp_noframe.s
        amd64/_setjmp.s
    )
elseif(${ARCH} STREQUAL "arm")
    list(APPEND VCRT_SETJMP_SOURCES
        arm/longjmp.c
    )
    list(APPEND VCRT_SETJMP_ASM_SOURCES
        arm/__longjmp_noframe.s
        arm/_setjmp.s
    )
elseif(${ARCH} STREQUAL "arm64")
    list(APPEND VCRT_SETJMP_SOURCES
        arm64/longjmp.c
    )
    list(APPEND VCRT_SETJMP_ASM_SOURCES
        #arm64/_setjmp.s
    )
endif()
add_asm_files(vcrt_setjmp_asm ${VCRT_SETJMP_ASM_SOURCES})

# Separate library, so we can link it to ftfd
add_library(setjmp ${VCRT_SETJMP_SOURCES} ${vcrt_setjmp_asm})
add_dependencies(setjmp psdk asm)


if(${ARCH} STREQUAL "i386")
    list(APPEND VCRUNTIME_SOURCES
        i386/__memmove_init.c
        i386/__memset_init.c
        i386/_chkesp_failed.c
    )
    list(APPEND VCRUNTIME_ASM_SOURCES
        i386/_chkesp.s
        i386/memmove.asm
        i386/memset.asm
    )
elseif(${ARCH} STREQUAL "amd64")
    list(APPEND VCRUNTIME_SOURCES
        amd64/_local_unwind.c
        amd64/chandler.c
        amd64/__memmove_init.c
        amd64/__memset_init.c
    )
    list(APPEND VCRUNTIME_ASM_SOURCES
        amd64/memmove.asm
        amd64/memset.asm
    )
else()
    list(APPEND VCRUNTIME_SOURCES
        memmove.c
        memset.c
    )
endif()
add_asm_files(vcruntime_asm ${VCRUNTIME_ASM_SOURCES})

list(APPEND VCRUNTIME_SOURCES
    ${vcruntime_asm}
    __std_terminate.c
    __vcrt_init.c
    memchr.c
    memcmp.c
    memcpy.c
    purecall.cpp
    strchr.c
    strrchr.c
    strstr.c
    wcschr.c
    wcsrchr.c
    wcsstr.c
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Prevent GCC from optimizing loops in memcpy/memmove/memset back into
    # calls to memcpy/memmove/memset, causing infinite recursion at -O2.
    set_source_files_properties(memcpy.c memmove.c memset.c
        PROPERTIES COMPILE_FLAGS "-fno-tree-loop-distribute-patterns")
endif()

# Runtime library (linked into ucrtbase)
add_library(vcruntime ${VCRUNTIME_SOURCES} $<TARGET_OBJECTS:setjmp>)
target_link_libraries(vcruntime ${PSEH_LIB})
target_include_directories(vcruntime INTERFACE inc)

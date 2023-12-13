

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)
spec2def(winecrt.dll winecrt.spec ADD_IMPORTLIB)


add_definitions(
    -DUSE_winecrt_PREFIX
    -D_winecrt_
    -D_winecrt_LIB_
    -D_MT
    -D_CTYPE_DISABLE_MACROS
    -D_NO_INLINING
    -DCRTDLL
    -D__MINGW_IMPORT="")

list(APPEND SOURCE
    dllmain.c
    stubs.c
    winecrt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winecrt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winecrt.def)

add_library(winecrt MODULE ${SOURCE})
set_module_type(winecrt win32dll ENTRYPOINT DllMain 12)
add_dependencies(winecrt psdk)
target_link_libraries(winecrt crt wine ${PSEH_LIB})
add_importlibs(winecrt kernel32 ntdll)
add_cd_file(TARGET winecrt DESTINATION reactos/system32 FOR all)

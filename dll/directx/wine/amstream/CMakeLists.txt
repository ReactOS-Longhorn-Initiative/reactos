
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(amstream.dll amstream.spec)

list(APPEND SOURCE
    audiodata.c
    audiostream.c
    ddrawstream.c
    filter.c
    main.c
    multimedia.c
    precomp.h)

add_library(amstream MODULE
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/amstream.def)

set_module_type(amstream win32dll)
target_link_libraries(amstream strmbase strmiids dxguid uuid wine_dll_register)
add_importlibs(amstream ole32 ddraw user32 msvcrt kernel32 ntdll)
add_pch(amstream precomp.h SOURCE)
add_cd_file(TARGET amstream DESTINATION reactos/system32 FOR all)

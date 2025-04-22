
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_PresentationDebug_)
spec2def(PresentationDebug.dll PresentationDebug.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    entry.c
    stubs.c)

add_library(PresentationDebug MODULE
    ${SOURCE}
    PresentationDebug.rc
    ${CMAKE_CURRENT_BINARY_DIR}/PresentationDebug.def
	${CMAKE_CURRENT_BINARY_DIR}/PresentationDebug_stubs.c)

set_module_type(PresentationDebug win32dll)
add_importlibs(PresentationDebug msvcrt kernel32 ntdll)
add_dependencies(PresentationDebug psdk)
add_cd_file(TARGET PresentationDebug DESTINATION reactos/system32 FOR all)
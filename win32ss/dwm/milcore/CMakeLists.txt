
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_milcore_)
spec2def(milcore.dll milcore.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    entry.c
    stubs.cpp)

add_library(milcore MODULE
    ${SOURCE}
    milcore.rc
    ${CMAKE_CURRENT_BINARY_DIR}/milcore.def
	${CMAKE_CURRENT_BINARY_DIR}/milcore_stubs.c)

set_module_type(milcore win32dll)
add_importlibs(milcore msvcrt kernel32 ntdll user32)
add_dependencies(milcore psdk)
#add_cd_file(TARGET milcore DESTINATION reactos/system32 FOR all)

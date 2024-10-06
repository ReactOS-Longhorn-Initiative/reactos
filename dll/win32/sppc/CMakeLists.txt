remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)


include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_sppc_)
spec2def(sppc.dll sppc.spec ADD_IMPORTLIB)
list(APPEND SOURCE
    sppc.c)

add_library(sppc MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/sppc.def
	${CMAKE_CURRENT_BINARY_DIR}/sppc_stubs.c)

set_module_type(sppc win32dll)
add_importlibs(sppc msvcrt kernel32 ntdll user32)
target_link_libraries(sppc wine)
add_dependencies(sppc psdk)
add_cd_file(TARGET sppc DESTINATION reactos/system32 FOR all)

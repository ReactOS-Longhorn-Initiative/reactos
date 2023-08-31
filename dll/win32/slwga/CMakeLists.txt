
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_slwga_)
spec2def(slwga.dll slwga.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    entry.c
    stubs.c)

add_library(slwga MODULE
    ${SOURCE}
    slwga.rc
    ${CMAKE_CURRENT_BINARY_DIR}/slwga.def
	${CMAKE_CURRENT_BINARY_DIR}/slwga_stubs.c)

set_module_type(slwga win32dll)
add_importlibs(slwga msvcrt kernel32 ntdll user32)
add_dependencies(slwga psdk)
add_cd_file(TARGET slwga DESTINATION reactos/system32 FOR all)
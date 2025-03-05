remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_slc_)
spec2def(slc.dll slc.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    slc.c)

add_library(slc MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/slc.def
	${CMAKE_CURRENT_BINARY_DIR}/slc_stubs.c)

    
target_link_libraries(slc wine)
set_module_type(slc win32dll)
add_importlibs(slc sppc msvcrt kernel32 ntdll user32)
add_dependencies(slc psdk)
add_cd_file(TARGET slc DESTINATION reactos/system32 FOR all)
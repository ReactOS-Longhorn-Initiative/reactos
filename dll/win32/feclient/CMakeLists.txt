remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_feclient_)
spec2def(feclient.dll feclient.spec ADD_IMPORTLIB)

add_library(feclient MODULE
    ${CMAKE_CURRENT_BINARY_DIR}/feclient.def
	${CMAKE_CURRENT_BINARY_DIR}/feclient_stubs.c)

    
target_link_libraries(feclient wine)
set_module_type(feclient win32dll)
add_importlibs(feclient msvcrt kernel32 ntdll user32)
add_dependencies(feclient psdk)
add_cd_file(TARGET feclient DESTINATION reactos/system32 FOR all)
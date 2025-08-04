
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(feclient.dll feclient.spec)

add_library(feclient MODULE
    ${CMAKE_CURRENT_BINARY_DIR}/feclient.def
    ${CMAKE_CURRENT_BINARY_DIR}/feclient_stubs.c)

set_module_type(feclient win32dll UNICODE ENTRYPOINT 0)
target_link_libraries(feclient uuid wine)
add_importlibs(feclient user32 kernel32 ntdll)
add_cd_file(TARGET feclient DESTINATION reactos/system32 FOR all)

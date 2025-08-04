
add_definitions(-D__WINESRC__)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(combase.dll combase.spec)

add_library(combase MODULE
    ${CMAKE_CURRENT_BINARY_DIR}/combase.def
    ${CMAKE_CURRENT_BINARY_DIR}/combase_stubs.c)

set_module_type(combase win32dll UNICODE ENTRYPOINT 0)
target_link_libraries(combase uuid wine)
add_importlibs(combase ole32 user32 kernel32 ntdll)
add_cd_file(TARGET combase DESTINATION reactos/system32 FOR all)

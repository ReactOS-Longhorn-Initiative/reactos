
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(feclient.dll feclient.spec)

list(APPEND SOURCE
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/feclient_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/feclient.def)

add_library(feclient SHARED ${SOURCE})
set_module_type(feclient win32dll)
target_link_libraries(feclient wine)
add_importlibs(feclient advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET feclient DESTINATION reactos/system32 FOR all)

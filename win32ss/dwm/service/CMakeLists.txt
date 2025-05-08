
spec2def(uxsms.dll uxsms.spec ADD_IMPORTLIB)

add_library(uxsms MODULE
    uxsms.c
    serv.c
    port.c
    ${CMAKE_CURRENT_BINARY_DIR}/uxsms_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/uxsms.def)

set_module_type(uxsms win32dll UNICODE)
target_link_libraries(uxsms wine)
add_importlibs(uxsms uxtheme advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET uxsms DESTINATION reactos/system32 FOR all)

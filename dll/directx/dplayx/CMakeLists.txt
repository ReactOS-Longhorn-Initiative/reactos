spec2def(dplayx.dll dplayx.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dplayx_main.c)

add_library(dplayx MODULE
    ${SOURCE}
    dplayx.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dplayx.def
    ${CMAKE_CURRENT_BINARY_DIR}/dplayx_stubs.c)

set_module_type(dplayx win32dll)
target_link_libraries(dplayx dxguid uuid)
add_importlibs(dplayx winmm ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dplayx DESTINATION reactos/system32 FOR all)

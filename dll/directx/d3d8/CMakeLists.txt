spec2def(d3d8.dll d3d8.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3d8.c)

add_library(d3d8 MODULE
    ${SOURCE}
    d3d8.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3d8_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3d8.def)

set_module_type(d3d8 win32dll)
target_link_libraries(d3d8 dxguid uuid)
add_importlibs(d3d8 d3d8thk advapi32 version user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET d3d8 DESTINATION reactos/system32 FOR all)

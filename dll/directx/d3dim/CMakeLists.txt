spec2def(d3dim.dll d3dim.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3dim.c)

add_library(d3dim MODULE
    ${SOURCE}
    d3dim.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dim_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dim.def)

set_module_type(d3dim win32dll)
target_link_libraries(d3dim dxguid uuid)
add_importlibs(d3dim advapi32 version user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET d3dim DESTINATION reactos/system32 FOR all)

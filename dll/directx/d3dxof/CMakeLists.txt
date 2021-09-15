spec2def(d3dxof.dll d3dxof.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c)

add_library(d3dxof MODULE
    ${SOURCE}
    d3dxof.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dxof.def
    ${CMAKE_CURRENT_BINARY_DIR}/d3dxof_stubs.c)

set_module_type(d3dxof win32dll)
target_link_libraries(d3dxof dxguid uuid)
add_importlibs(d3dxof msvcrt kernel32 ntdll)
add_cd_file(TARGET d3dxof DESTINATION reactos/system32 FOR all)

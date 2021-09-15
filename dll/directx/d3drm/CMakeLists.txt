spec2def(d3drm.dll d3drm.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3drm_main.c)

add_library(d3drm MODULE
    ${SOURCE}
    d3drm.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3drm_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3drm.def)

set_module_type(d3drm win32dll UNICODE)
target_link_libraries(d3drm dxguid uuid)
add_importlibs(d3drm ddraw d3dxof msvcrt kernel32 ntdll)
add_pch(d3drm precomp.h SOURCE)
add_cd_file(TARGET d3drm DESTINATION reactos/system32 FOR all)

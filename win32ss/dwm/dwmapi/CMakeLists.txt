
spec2def(dwmapi.dll dwmapi.spec)

list(APPEND SOURCE
     entry)

add_library(dwmapi MODULE
    ${SOURCE}
    dwmapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi.def
    ${CMAKE_CURRENT_BINARY_DIR}/dwmapi_stubs.c)

set_module_type(dwmapi win32dll UNICODE ENTRYPOINT 0)
target_link_libraries(dwmapi uuid)
add_importlibs(dwmapi user32 kernel32 ntdll)
add_cd_file(TARGET dwmapi DESTINATION reactos/system32 FOR all)


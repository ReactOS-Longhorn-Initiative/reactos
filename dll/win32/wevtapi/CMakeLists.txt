
add_definitions(-D_wevtapi_)

spec2def(wevtapi.dll wevtapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c
    KB.h)

add_library(wevtapi MODULE
    ${SOURCE}
    wevtapi.rc
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wevtapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wevtapi.def)

target_link_libraries(wevtapi)
set_module_type(wevtapi win32dll ENTRYPOINT DllMain)
add_importlibs(wevtapi ntdll ntdll_vista kernel32_vista advapi32 kernel32)
add_dependencies(wevtapi psdk)
add_cd_file(TARGET wevtapi DESTINATION reactos/system32 FOR all)
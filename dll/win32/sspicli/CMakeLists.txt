
add_definitions(-D_sspicli_)

spec2def(sspicli.dll sspicli.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c
    KB.h)

add_library(sspicli MODULE
    ${SOURCE}
    sspicli.rc
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sspicli_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sspicli.def)

target_link_libraries(sspicli)
set_module_type(sspicli win32dll ENTRYPOINT DllMain)
add_importlibs(sspicli ntdll ntdll_vista kernel32_vista advapi32 kernel32)
add_dependencies(sspicli psdk)
add_cd_file(TARGET sspicli DESTINATION reactos/system32 FOR all)
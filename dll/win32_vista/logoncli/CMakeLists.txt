
add_definitions(-D_logoncli_)

spec2def(logoncli.dll logoncli.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c
    KB.h)

add_library(logoncli MODULE
    ${SOURCE}
    logoncli.rc
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/logoncli_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/logoncli.def)

target_link_libraries(logoncli)
set_module_type(logoncli win32dll ENTRYPOINT DllMain)
add_importlibs(logoncli ntdll ntdll_vista kernel32_vista advapi32 kernel32)
add_dependencies(logoncli psdk)
add_cd_file(TARGET logoncli DESTINATION reactos/system32 FOR all)
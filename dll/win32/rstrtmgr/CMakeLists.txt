
add_definitions(-D_RstrtMgr_)

spec2def(RstrtMgr.dll RstrtMgr.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c
    KB.h)

add_library(RstrtMgr MODULE
    ${SOURCE}
    RstrtMgr.rc
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/RstrtMgr_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/RstrtMgr.def)

target_link_libraries(RstrtMgr)
set_module_type(RstrtMgr win32dll ENTRYPOINT DllMain)
add_importlibs(RstrtMgr ntdll ntdll_vista kernel32_vista advapi32 kernel32)
add_dependencies(RstrtMgr psdk)
add_cd_file(TARGET RstrtMgr DESTINATION reactos/system32 FOR all)

add_definitions(-D_ktmw32_)

spec2def(ktmw32.dll ktmw32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DllMain.c
    KB.h)

add_library(ktmw32 MODULE
    ${SOURCE}
    ktmw32.rc
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ktmw32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ktmw32.def)

target_link_libraries(ktmw32)
set_module_type(ktmw32 win32dll ENTRYPOINT DllMain)
add_importlibs(ktmw32 ntdll ntdll_vista kernel32_vista advapi32 kernel32)
add_dependencies(ktmw32 psdk)
add_cd_file(TARGET ktmw32 DESTINATION reactos/system32 FOR all)

add_definitions(-D_KTM32_)

spec2def(ktmw32.dll ktmw32.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ktmw32_main.c)

add_library(ktmw32 MODULE
    ${SOURCE}
    ktmw32.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ktmw32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ktmw32.def)

target_link_libraries(ktmw32)
set_module_type(ktmw32 win32dll ENTRYPOINT DllMain)
add_importlibs(ktmw32 ntdll ntdll_vista advapi32 kernelbase)
add_dependencies(ktmw32 psdk)
add_cd_file(TARGET ktmw32 DESTINATION reactos/system32 FOR all)

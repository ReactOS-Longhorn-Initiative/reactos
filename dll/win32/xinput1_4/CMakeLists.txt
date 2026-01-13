
spec2def(xinput1_4.dll xinput1_4.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    xinput1_4_main.c
     ${CMAKE_CURRENT_BINARY_DIR}/xinput1_4_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/xinput1_4.def)

add_library(xinput1_4 MODULE ${SOURCE} version.rc)
set_module_type(xinput1_4 win32dll)
target_link_libraries(xinput1_4 wine)
add_importlibs(xinput1_4 msvcrt kernel32 ntdll)
add_cd_file(TARGET xinput1_4 DESTINATION reactos/system32 FOR all)
set_wine_module(xinput1_4)

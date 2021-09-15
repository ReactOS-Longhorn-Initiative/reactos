spec2def(dinput8.dll dinput8.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dinput8.c
    ${CMAKE_CURRENT_BINARY_DIR}/dinput8.def)

add_library(dinput8 MODULE
    ${SOURCE}
    dinput8.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dinput8.def
    ${CMAKE_CURRENT_BINARY_DIR}/dinput8_stubs.c)

set_module_type(dinput8 win32dll)
target_link_libraries(dinput8 dxguid uuid)
add_importlibs(dinput8 comctl32 ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dinput8 DESTINATION reactos/system32 FOR all)

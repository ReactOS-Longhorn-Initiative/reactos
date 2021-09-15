
spec2def(dinput.dll dinput.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dinput_main.c)

add_library(dinput MODULE
    ${SOURCE}
    dinput.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dinput.def
    ${CMAKE_CURRENT_BINARY_DIR}/dinput_stubs.c)

set_module_type(dinput win32dll)
target_link_libraries(dinput dxguid uuid)
add_importlibs(dinput comctl32 ole32 user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dinput DESTINATION reactos/system32 FOR all)

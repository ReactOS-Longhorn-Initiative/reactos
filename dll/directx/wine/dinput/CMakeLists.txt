
add_definitions(-DDIRECTINPUT_VERSION=0x0700)
spec2def(dinput.dll dinput.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ansi.c
    config.c
    device.c
    dinput_main.c
    joystick_hid.c
    keyboard.c
    mouse.c
    dllmain.c)

add_library(dinput MODULE
    ${SOURCE}
    data_formats.c
    dinput.rc
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dinput.def)
set_wine_module_FIXME(dinput) # CORE-5743: No ARRAY_SIZE macro

if(MSVC)
    # Disable C5286: implicit conversion from enum type 'x' to enum type 'y';
    target_compile_options(dinput PRIVATE /wd4047 /wd4024)
endif()


if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(dinput PRIVATE -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare -Wno-switch -Wno-write-strings -Wno-format)
endif()

add_library(dinput_data_formats data_formats.c)
add_dependencies(dinput_data_formats psdk)
set_wine_module_FIXME(dinput_data_formats) # CORE-5743: No ARRAY_SIZE macro

set_module_type(dinput win32dll)
target_link_libraries(dinput dxguid uuid wine)
add_importlibs(dinput comctl32 ole32 setupapi hid user32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dinput DESTINATION reactos/system32 FOR all)

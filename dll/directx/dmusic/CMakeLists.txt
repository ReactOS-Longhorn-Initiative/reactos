spec2def(dmusic.dll dmusic.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dmusic_main.c)

add_library(dmusic MODULE
    ${SOURCE}
    dmusic.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dmusic.def
    ${CMAKE_CURRENT_BINARY_DIR}/dmusic_stubs.c)

set_module_type(dmusic win32dll)
target_link_libraries(dmusic dxguid uuid)
add_importlibs(dmusic ole32 advapi32 winmm dsound user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dmusic DESTINATION reactos/system32 FOR all)

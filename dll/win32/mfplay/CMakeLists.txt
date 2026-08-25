
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(mfplay.dll mfplay.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    player.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfplay_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfplay.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(mfplay MODULE ${SOURCE})
set_module_type(mfplay win32dll)
target_link_libraries(mfplay wine wine_dll_register wine_dll_canunload uuid mfuuid strmiids oldnames)
add_importlibs(mfplay mfplat mf ole32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mfplay DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mfplay) # CORE-5743: ???
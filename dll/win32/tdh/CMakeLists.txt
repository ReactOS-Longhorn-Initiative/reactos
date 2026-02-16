
spec2def(tdh.dll tdh.spec)

list(APPEND SOURCE
    tdh_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/tdh_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/tdh.def)

add_library(tdh MODULE ${SOURCE} version.rc)
set_module_type(tdh win32dll)
target_link_libraries(tdh wine)
add_importlibs(tdh msvcrt kernel32 ntdll)
add_cd_file(TARGET tdh DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(tdh) # CORE-5743: ???


spec2def(cryptbase.dll cryptbase.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cryptbase_main.c
    des.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptbase_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptbase.def)

add_library(cryptbase MODULE ${SOURCE})
set_module_type(cryptbase win32dll ENTRYPOINT 0)
target_link_libraries(cryptbase wine)
add_importlibs(cryptbase msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptbase DESTINATION reactos/system32 FOR all)
set_wine_module(cryptbase)


add_definitions(
    -DUSE_WINE_TODOS
    -D__WINESRC__)

add_executable(wmvcore_winetest wmvcore.c testlist.c rsrc.rc)
set_module_type(wmvcore_winetest win32cui)
add_importlibs(wmvcore_winetest ole32 wmvcore user32 msvcrt kernel32)
add_rostests_file(TARGET wmvcore_winetest)

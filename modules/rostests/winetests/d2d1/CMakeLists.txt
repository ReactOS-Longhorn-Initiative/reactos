
add_definitions(-DUSE_WINE_TODOS -D_USE_MATH_DEFINES -DWINETEST_USE_DBGSTR_LONGLONG)
add_executable(d2d1_winetest d2d1.c testlist.c)
target_link_libraries(d2d1_winetest uuid dxguid)
set_module_type(d2d1_winetest win32cui)
add_importlibs(d2d1_winetest ddraw d3drm user32 msvcrt kernel32)
add_rostests_file(TARGET d2d1_winetest)

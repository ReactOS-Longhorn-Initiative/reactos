
remove_definitions(-D__ROS_LONG64__)
add_definitions(-DUSE_WINE_TODOS -DWINETEST_USE_DBGSTR_LONGLONG -D__WINESRC__)

list(APPEND SOURCE tdh.c)

add_executable(tdh_winetest ${SOURCE})
target_link_libraries(tdh_winetest uuid)
set_module_type(tdh_winetest win32cui)
add_importlibs(tdh_winetest msvcrt kernel32)

if(MSVC)
    add_importlibs(tdh_winetest ntdll)
endif()

add_rostests_file(TARGET tdh_winetest)

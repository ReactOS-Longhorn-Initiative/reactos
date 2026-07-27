

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/wine)
add_compile_definitions(-DWINETEST_USE_DBGSTR_LONGLONG)

list(APPEND SOURCE
    analyzer.c
    font.c
    layout.c
    resource.rc)

list(APPEND PCH_SKIP_SOURCE
    testlist.c)

add_executable(dwrite_winetest
    ${SOURCE}
    ${PCH_SKIP_SOURCE})


if (MSVC)
    # Disable warning C5287: operands are different enum types
    # Disable warning C5286: implicit conversion from enum type to different enum type
    target_compile_options(dwrite_winetest PRIVATE /wd5287 /wd5286)
endif()

target_link_libraries(dwrite_winetest uuid dxguid)
add_importlibs(dwrite_winetest ole32 gdi32 kernel32_vista dwrite user32 msvcrt kernel32)
set_module_type(dwrite_winetest win32cui)
add_rostests_file(TARGET dwrite_winetest)

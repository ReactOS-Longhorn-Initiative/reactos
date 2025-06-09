
add_definitions(
    -D__WINESRC__
	-Dwcsdup=_wcsdup)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/psdk)

spec2def(d2d1.dll d2d1.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    bitmap.c
	bitmap_render_target.c
	brush.c
	command_list.c
	dc_render_target.c
	device.c
	effect.c
	factory.c
	geometry.c
	hwnd_render_target.c
	layer.c
	guid.c
	mesh.c
	state_block.c
	stroke.c
	wic_render_target.c
    d2d1_private.h)

add_library(d2d1 SHARED
    ${SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d2d1_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/d2d1.def)
add_dependencies(d2d1 wineheaders d3d_idl_headers)

if(MSVC)
    # Disable warning C4146: unary minus operator applied to unsigned type, result still unsigned
    # Disable warning C4267: '=': conversion from 'size_t' to 'WORD', possible loss of data
    # Disable warning C4477 (printf format warnings)
    target_compile_options(d2d1 PRIVATE /wd4146 /wd4267 /wd4477)
endif()
set_module_type(d2d1 win32dll)
target_link_libraries(d2d1 wine dxguid dx10guid uuid)
add_importlibs(d2d1 xmllite d3dcompiler_43 d3d10_1 ucrtbase msvcrt dwrite ole32 gdi32 user32 kernel32 ntdll advapi32)
add_cd_file(TARGET d2d1 DESTINATION reactos/system32 FOR all)
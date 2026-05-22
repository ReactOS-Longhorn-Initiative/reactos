
spec2def(d3dx10_43.dll d3dx10_43.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ../d3dx9_36/animation.c
    ../d3dx9_36/core.c
    ../d3dx9_36/effect.c
    ../d3dx9_36/font.c
    ../d3dx9_36/line.c
    #../d3dx9_36/main.c
    ../d3dx9_36/math.c
    ../d3dx9_36/mesh.c
    ../d3dx9_36/preshader.c
    ../d3dx9_36/render.c
    ../d3dx9_36/shader.c
    ../d3dx9_36/skin.c
    ../d3dx9_36/sprite.c
    ../d3dx9_36/surface.c
    ../d3dx9_36/texture.c
    ../d3dx9_36/txc_compress_dxtn.c
    ../d3dx9_36/txc_fetch_dxtn.c
    ../d3dx9_36/util.c
    ../d3dx9_36/volume.c
    ../d3dx9_36/xfile.c
    ../d3dx10_43/async.c
    ../d3dx10_43/compiler.c
    ../d3dx10_43/font.c
    ../d3dx10_43/mesh.c
    ../d3dx10_43/sprite.c
    ../d3dx10_43/texture.c
    ../d3dx10_43/d3dx10_43_main.c)

list(APPEND PCH_SKIP_SOURCE
    ../d3dx9_36/guid.c
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx10_43_stubs.c)

add_library(d3dx10_43 MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dx10_43.def)


remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-D__ROS_LONG64__)
set_module_type(d3dx10_43 win32dll)
add_dependencies(d3dx10_43 d3d_idl_headers)
target_link_libraries(d3dx10_43 dxguid dx10guid wine oldnames)
add_importlibs(d3dx10_43 d3d10_1 d3dcompiler_43 d3dx9_36 d3dxof usp10 user32 ole32 gdi32 msvcrt kernelbase kernel32 ntdll)
add_delay_importlibs(d3dx10_43 windowscodecs)
  #  add_pch(d3dx10_43 ../d3dx11_43/precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET d3dx10_43 DESTINATION reactos/system32 FOR all)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(d3dx10_43 PRIVATE -Wno-incompatible-pointer-types -Wno-unused-but-set-variable -Wno-switch -Wno-error) # Our favourite compiler :)
endif()
target_compile_definitions(d3dx10_43 PRIVATE D3DX_SDK_VERSION=43 __WINESRC__ copysignf=_copysignf)

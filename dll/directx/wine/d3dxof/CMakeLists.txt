

add_definitions(-D__WINESRC__ -D__ROS_LONG64__ -Dstricmp=_stricmp)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/wine)
spec2def(d3dxof.dll d3dxof.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    d3dxof.c
    main.c
    parsing.c)

list(APPEND PCH_SKIP_SOURCE
    mszip.c)

add_library(d3dxof MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/d3dxof.def)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(d3dxof PRIVATE -Wno-format -Wno-incompatible-pointer-types -Wno-unused-but-set-variable -Wno-switch -Wno-error) # Our favourite compiler :)
endif()

set_module_type(d3dxof win32dll)
target_link_libraries(d3dxof wine_dll_register dxguid uuid wine)
add_importlibs(d3dxof msvcrt kernel32 kernelbase ntdll)
add_pch(d3dxof precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET d3dxof DESTINATION reactos/system32 FOR all)

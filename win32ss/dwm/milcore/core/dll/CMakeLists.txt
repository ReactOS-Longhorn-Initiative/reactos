set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/UtilLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/include)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore/common/DirectXLayer/)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore/common/DirectXLayer/Factories)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore/common/DirectXLayer/interfaces)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore/common/)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore/core/)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include/Generated)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include/processed)

add_definitions(-DD3D9EXTENSIONS) # todo should DIRECTXMATH be used?

add_definitions(-D_milcore_)
spec2def(milcore.dll milcore.spec ADD_IMPORTLIB)

list(APPEND SOURCE
precomp.cpp
dllentry.cpp
milcore.def
    )

add_library(milcore MODULE ${SOURCE})
add_dependencies(milcore xdk)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(milcore PRIVATE /wd5043 /wd5033)
endif()

if(NOT MSVC)
target_compile_options(milcore PRIVATE -Wno-comment -Wno-sign-compare -Wno-uninitialized)
endif()

set_target_cpp_properties(milcore WITH_EXCEPTIONS WITH_RTTI)
set_module_type(milcore win32dll UNICODE)
target_link_libraries(milcore milcoreCommon cppstl uuid dxguid UtilLib DebugLib commonShared targets hw gcutil glyph swlib milcoreMeta milcoreApi geometry effects scanop uce milcoreResources PixelShader Collector Platform Compiler)
add_importlibs(milcore winmm ole32 oleaut32 windowscodecs d3dx9_40 d3d10 d3d10_1 advapi32 user32 gdi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET milcore DESTINATION reactos/system32 FOR all)
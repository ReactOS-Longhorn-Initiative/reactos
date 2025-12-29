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
target_compile_options(milcore PUBLIC /wd5043 /wd5033)
endif()

if(NOT MSVC)
target_compile_options(milcore PUBLIC -Wno-comment -Wno-sign-compare -Wno-uninitialized)
endif()

set_target_cpp_properties(milcore WITH_EXCEPTIONS WITH_RTTI)
set_module_type(milcore win32dll UNICODE)

list(APPEND milcoreLibs dxguid uce milcoreApi swlib hw milcoreCommon commonShared targets gcutil glyph milcoreMeta effects scanop milcoreResources PixelShader geometry Collector Platform Compiler libgdi32 UtilLib DebugLib)

# TODO: Once CMake is upgraded to 3.24+, use LINK_GROUP
if(MSVC)
target_link_libraries(milcore cppstl uuid ${milcoreLibs})
else()
target_link_libraries(milcore cppstl uuid -Wl,--start-group ${milcoreLibs} "-Wl,--end-group")
endif()

add_importlibs(milcore ole32 oleaut32 windowscodecs advapi32 user32 winmm d3dx9_36 d3d10 d3d10_1 msvcrt kernel32 ntdll)
add_cd_file(TARGET milcore DESTINATION reactos/system32 FOR all)
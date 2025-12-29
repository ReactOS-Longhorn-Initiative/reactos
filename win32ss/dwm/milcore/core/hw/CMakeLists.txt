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

list(APPEND SOURCE
bitmapofdevicebitmaps.cpp
d3ddevice.cpp
d3ddevicemanager.cpp
d3dgeometry.cpp
d3dglyphbank.cpp
d3dglyphpainter.cpp
d3dglyphrun.cpp
d3dlockabletexture.cpp
d3dlog.cpp
d3dregistry.cpp
d3drenderstate.cpp
d3dresource.cpp
d3dstats.cpp
d3dsurface.cpp
d3dswapchain.cpp
d3dswapchainwithswdc.cpp
d3dtexture.cpp
D3DTextureSurface.cpp
d3dvidmemonlytexture.cpp
gpumarker.cpp
hw3dgeometrystreambuffer.cpp
HwBitBltDeviceBitmapColorSource.cpp
HwBitmapBrush.cpp
HwBitmapCache.cpp
hwbitmapcolorsource.cpp
HwBoxColorSource.cpp
HwBrushContext.cpp
HwBrushPool.cpp
HwColorComponentColorSource.cpp
HwConstantColorSource.cpp
hwdestinationtexture.cpp
hwdestinationtexturepool.cpp
hwdevicebitmapcolorsource.cpp
hwdisplayrt.cpp
hwffshaders.cpp
HwHLSLShaderFragments.cpp
hwhwndrt.cpp
hwinit.cpp
HwLightingColorSource.cpp
HwLinearGradientBrush.cpp
hwlineargradientcolorsource.cpp
hwpipeline.cpp
hwpipelinebuilder.cpp
HwRadialGradientBrush.cpp
HwRadialGradientColorSource.cpp
hwrasterizer.cpp
HwRenderStateManager.cpp
hwshader.cpp
HwShaderBuilder.cpp
HwShaderCache.cpp
HwShaderEffect.cpp
hwshaderfragmenttohlslconverter.cpp
HwShaderPipeline.cpp
HwSolidBrush.cpp
hwsolidcolortexturesource.cpp
hwsolidcolortexturesourcepool.cpp
HwStateTable.cpp
hwsurfrt.cpp
hwsurfrtdata.cpp
hwsw3dfallback.cpp
hwtexturedcolorsource.cpp
HwTexturedColorSourceBrush.cpp
hwtexturert.cpp
HwUtils.cpp
hwvertexbuffer.cpp
HwVidMemTextureManager.cpp
InteropDeviceBitmap.cpp
precomp.cpp
swfallback.cpp
waffler.cpp
    )

add_library(hw ${SOURCE})
add_dependencies(hw xdk)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(hw PRIVATE /wd5043 /wd5033 /wd4838)
endif()

if(NOT MSVC)
target_compile_options(hw PRIVATE -Wno-comment -Wno-sign-compare -Wno-class-memaccess -Wno-uninitialized -Wno-reorder -Wno-delete-non-virtual-dtor -Wno-switch -Wno-unused-but-set-variable -Wno-parentheses -Wno-unused-variable -Wno-narrowing)
endif()
#add_pch(hw precomp.cpp SOURCE)

set_target_cpp_properties(hw WITH_EXCEPTIONS WITH_RTTI)

target_compile_options(hw PRIVATE -DUNICODE -D_UNICODE)

target_link_libraries(hw PRIVATE cppstl)
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
affinetransform3d.cpp
ambientlight.cpp
axisanglerotation3d.cpp
BitmapCacheBrush.cpp
BitmapCacheMode.cpp
bitmapres.cpp
BlurEffect.cpp
brush.cpp
BrushIntermediateCache.cpp
BrushIntermediateRealizer.cpp
BrushRealizer.cpp
BrushTypeUtils.cpp
CacheMode.cpp
camera.cpp
combinedgeometry.cpp
CurrentValue.cpp
d3dimage.cpp
DashStyle.cpp
DeviceAlignedIntermediateRealizer.cpp
diffusematerial.cpp
directionallight.cpp
doublebufferedbitmapres.cpp
drawingbrush.cpp
drawinggroup.cpp
drawingimage.cpp
DropShadowEffect.cpp
Effect.cpp
ellipsegeometry.cpp
emissivematerial.cpp
etwresource.cpp
geometry.cpp
geometry3d.cpp
GeometryDrawing.cpp
GeometryGroup.cpp
geometrymodel3d.cpp
GlyphRunDrawing.cpp
GlyphRunGeometrySink.cpp
glyphrunslave.cpp
gradientbrush.cpp
guidelinecollectionresource.cpp
imagebrush.cpp
ImageDrawing.cpp
ImplicitInputBrush.cpp
light.cpp
lineargradient.cpp
linegeometry.cpp
marshal_generated.cpp
material.cpp
materialgroup.cpp
matrix.cpp
matrixcamera.cpp
matrixtransform3d.cpp
meshgeometry3d.cpp
model3d.cpp
model3dgroup.cpp
modelrenderwalker.cpp
modelwalker.cpp
node.cpp
#orthographiccamera.cpp
pathgeometry.cpp
PathGeometryWrapper.cpp
pen.cpp
perspectivecamera.cpp
PixelShader.cpp
pointlight.cpp
precomp.cpp
prerenderwalker.cpp
ProjectionCamera.cpp
QuaternionRotation3D.cpp
radialgradient.cpp
rectanglegeometry.cpp
renderdata.cpp
renderdata_generated.cpp
rotate.cpp
rotatetransform3d.cpp
rotation3d.cpp
scale.cpp
scaletransform3d.cpp
ShaderEffect.cpp
skew.cpp
solidcolorbrush.cpp
specularmaterial.cpp
spotlight.cpp
tilebrush.cpp
TileBrushUtils.cpp
transform.cpp
transform3d.cpp
transform3dgroup.cpp
transformgroup.cpp
translate.cpp
translatetransform3d.cpp
valueres.cpp
VideoDrawing.cpp
videoslave.cpp
viewport3dvisual.cpp
ViewportAlignedIntermediateRealizer.cpp
visual3d.cpp
visualbrush.cpp
)

add_library(milcoreResources ${SOURCE})
add_dependencies(milcoreResources xdk)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(milcoreResources PRIVATE /wd5043 /wd5033 /wd4838 /Zc:sizedDealloc-)
endif()

if(NOT MSVC)
target_compile_options(milcoreResources PRIVATE -Wno-comment -Wno-sign-compare -Wno-class-memaccess -Wno-uninitialized -Wno-reorder -Wno-delete-non-virtual-dtor -Wno-switch -Wno-unused-but-set-variable -Wno-parentheses -Wno-unused-variable -Wno-narrowing -Wno-invalid-offsetof)
endif()
#add_pch(milcoreResources precomp.cpp SOURCE)

set_target_cpp_properties(milcoreResources WITH_EXCEPTIONS WITH_RTTI)

target_compile_options(milcoreResources PRIVATE -DUNICODE -D_UNICODE)

target_link_libraries(milcoreResources PRIVATE cppstl)
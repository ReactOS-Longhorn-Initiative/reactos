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
3dutils.cpp
AssertDllInUse.cpp
BaseMatrix.cpp
BufferDispenser.cpp
d3dloader.cpp
d3dutils.cpp
display.cpp
Dump.cpp
dwminterop.cpp
dwritefactory.cpp
engine.cpp
EnhancedContrastTable.cpp
exports.cpp
Gamma.cpp
gradienttexture.cpp
guidelinecollection.cpp
lazymem.cpp
log.cpp
matrix3x2.cpp
matrix.cpp
MatrixStack.cpp
memblock.cpp
memblockreader.cpp
memreader.cpp
memwriter.cpp
milboxf.cpp
milcoredllentry.cpp
mildc.cpp
MILMatrix.cpp
oscompat.cpp
precomp.cpp
ptrarray.cpp
ptrset.cpp
RectUtils.cpp
renderoptions.cpp
ResourcePool.cpp
rtutils.cpp
slistutil.cpp
Tier.cpp
utils.cpp
WatermarkStack.cpp
    )

add_library(milcoreCommon ${SOURCE})
add_dependencies(milcoreCommon xdk)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(milcoreCommon PRIVATE /wd5043 /wd5033)
endif()

if(NOT MSVC)
# Disable MilRect<int, tagRECT, RectUniqueness::_CMILSurfaceRect_>::sc_rcInfinite' to be defined as 'TMilRect_<int, tagRECT, MilPointAndSizeL, RectUniqueness::_CMILSurfaceRect_>::sc_rcInfinite
get_target_property(target_options milcoreCommon COMPILE_OPTIONS)
list(REMOVE_ITEM target_options "-Werror")
set_property(TARGET milcoreCommon PROPERTY COMPILE_OPTIONS ${target_options})

target_compile_options(milcoreCommon PRIVATE -Wno-comment -Wno-sign-compare -Wno-class-memaccess -Wno-uninitialized -fpermissive)
endif()
#add_pch(milcoreCommon precomp.cpp SOURCE)

set_target_cpp_properties(milcoreCommon WITH_EXCEPTIONS WITH_RTTI)

target_compile_options(milcoreCommon PRIVATE -DUNICODE -D_UNICODE)

target_link_libraries(milcoreCommon PRIVATE cppstl)

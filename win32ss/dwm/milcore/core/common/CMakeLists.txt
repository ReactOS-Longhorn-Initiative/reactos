set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

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

add_library(common ${SOURCE})
add_dependencies(common xdk)

target_link_libraries(common PRIVATE cppstl)
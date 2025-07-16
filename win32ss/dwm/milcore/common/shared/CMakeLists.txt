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
AssertEntry.cpp
dynarrayimpl.cpp
engine.cpp
etwtrace.cpp
GammaLUTs.cpp
mem.cpp
milcom.cpp
MILRect.cpp
MILRectF_WH.cpp
pixelformatutils.cpp
precomp.cpp
processorfeatures.cpp
real.cpp
RefCountBase.cpp
ResourceCache.cpp
rgnutils.cpp
utc.cpp
utils.cpp
xmm.cpp
    )

add_library(commonShared ${SOURCE})
add_dependencies(commonShared xdk)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(commonShared PRIVATE /wd5043 /wd5033)
endif()

if(NOT MSVC)
# Disable MilRect<int, tagRECT, RectUniqueness::_CMILSurfaceRect_>::sc_rcInfinite' to be defined as 'TMilRect_<int, tagRECT, MilPointAndSizeL, RectUniqueness::_CMILSurfaceRect_>::sc_rcInfinite
get_target_property(target_options commonShared COMPILE_OPTIONS)
list(REMOVE_ITEM target_options "-Werror")
set_property(TARGET commonShared PROPERTY COMPILE_OPTIONS ${target_options})

target_compile_options(commonShared PRIVATE -Wno-comment -Wno-sign-compare -Wno-class-memaccess -Wno-uninitialized -Wno-switch -Wno-reorder -fpermissive)
endif()

set_target_cpp_properties(commonShared WITH_EXCEPTIONS WITH_RTTI)

target_compile_options(commonShared PRIVATE -DUNICODE -D_UNICODE)

target_link_libraries(commonShared PRIVATE cppstl)

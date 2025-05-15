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
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore/core/sw/)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include/Generated)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include/processed)

add_definitions(-DD3D9EXTENSIONS) # todo should DIRECTXMATH be used?

list(APPEND SOURCE
AnimationPath.cpp
area.cpp
AxisAlignedStripClipper.cpp
bezier.cpp
BezierD.cpp
bezierflattener.cpp
Boolean.cpp
compactshapes.cpp
cpen.cpp
ExactArithmetic.cpp
figure.cpp
FigureBase.cpp
FigureTask.cpp
FillTessellator.cpp
LineSegmentIntersection.cpp
precomp.cpp
scanner.cpp
shape.cpp
shapebase.cpp
StripClipper.cpp
strokefigure.cpp
tessellate.cpp
utils.cpp)

add_library(geometry ${SOURCE})
add_dependencies(geometry xdk)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(geometry PRIVATE /wd5043 /wd5033 /wd4838 /Zc:sizedDealloc-)
endif()

if(NOT MSVC)
target_compile_options(geometry PRIVATE -Wno-comment -Wno-sign-compare -Wno-class-memaccess -Wno-uninitialized -Wno-reorder -Wno-delete-non-virtual-dtor -Wno-switch -Wno-unused-but-set-variable -Wno-parentheses -Wno-unused-variable -Wno-narrowing)
endif()
#add_pch(geometry precomp.cpp SOURCE)

set_target_cpp_properties(geometry WITH_EXCEPTIONS WITH_RTTI)

target_compile_options(geometry PRIVATE -DUNICODE -D_UNICODE)

target_link_libraries(geometry PRIVATE cppstl)
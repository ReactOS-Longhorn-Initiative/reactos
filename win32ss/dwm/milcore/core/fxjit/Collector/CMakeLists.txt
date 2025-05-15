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
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore/core/fxjit/Public)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore/core/fxjit/Compiler)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include/Generated)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include/processed)

add_definitions(-DD3D9EXTENSIONS) # todo should DIRECTXMATH be used?

list(APPEND SOURCE
Branch.cpp
C_f32x1.cpp
C_f32x4.cpp
C_LazyVar.cpp
C_s16x8.cpp
C_s32x4.cpp
C_u128x1.cpp
C_u16x4.cpp
C_u16x8.cpp
C_u32.cpp
C_u32x2.cpp
C_u32x4.cpp
C_u64.cpp
C_u64x1.cpp
C_u64x2.cpp
C_u8x16.cpp
C_u8x8.cpp
jitteraccess.cpp
MmValue.cpp
precomp.cpp
PVOID.cpp
P_f32x4.cpp
XmmValue.cpp
)

add_library(Collector ${SOURCE})
add_dependencies(Collector xdk)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(Collector PRIVATE /wd5043 /wd5033 /wd4838)
endif()

if(NOT MSVC)
target_compile_options(Collector PRIVATE -Wno-comment -Wno-sign-compare -Wno-class-memaccess -Wno-uninitialized -Wno-reorder -Wno-delete-non-virtual-dtor -Wno-switch -Wno-unused-but-set-variable -Wno-parentheses -Wno-unused-variable -Wno-narrowing)
endif()
#add_pch(Collector precomp.cpp SOURCE)

set_target_cpp_properties(Collector WITH_EXCEPTIONS WITH_RTTI)

target_compile_options(Collector PRIVATE -DUNICODE -D_UNICODE)

target_link_libraries(Collector PRIVATE cppstl)
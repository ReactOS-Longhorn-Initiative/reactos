set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/rwm)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/LpcConnectLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/LpcCreateLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/include)
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

# DIRECTXMATH requires SSE
add_definitions(-DD3D9EXTENSIONS) # todo should DD3D9EXTENSIONS / DIRECTXMATH be used?

list(APPEND SOURCE
dwmstartup.cpp
)

add_library(dwmredir ${SOURCE})
add_dependencies(dwmredir xdk)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(dwmredir PRIVATE /wd5043)
endif()

set_target_cpp_properties(dwmredir WITH_EXCEPTIONS WITH_RTTI)

target_link_libraries(dwmredir PRIVATE cppstl)

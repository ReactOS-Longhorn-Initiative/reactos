set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/UtilLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/include)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include)

list(APPEND SOURCE
    DebugLib.cxx
    )

add_library(DebugLib ${SOURCE})
add_dependencies(DebugLib xdk)

target_link_libraries(DebugLib PRIVATE cppstl)
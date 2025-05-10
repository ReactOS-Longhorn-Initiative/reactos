set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/UtilLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/dwm/shared/LpcConnectLib)

list(APPEND SOURCE
    LpcConnectLib.cpp
    )

add_library(LpcConnectLib ${SOURCE})
add_dependencies(LpcConnectLib xdk)

target_link_libraries(LpcConnectLib PRIVATE cppstl)

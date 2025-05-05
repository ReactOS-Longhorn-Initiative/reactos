set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/UtilLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include)

list(APPEND SOURCE
    Assert.cxx
    DebugBreak.cpp
    HiResTimer.cxx
    Instrumentation.cxx
    list.cxx
    MemUtils.cxx
    Misc.cxx
    pch.cxx
    Registry.cxx
    StrUtil.cxx
    )

add_library(UtilLib ${SOURCE})
add_dependencies(UtilLib xdk)

if(NOT MSVC)
    target_compile_options(UtilLib PRIVATE -frtti)
endif()

target_link_libraries(UtilLib PRIVATE cppstl)
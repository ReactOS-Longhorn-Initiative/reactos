
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)
add_definitions(-DD3D9EXTENSIONS) #
include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/rwm)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/include)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/UtilLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include/Generated)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_include/processed)

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
add_definitions(-D_UDWM_)
add_definitions(-DUDWM_TARGET_VISTA_SP1_MILCORE=1)
spec2def(uDWM.dll uDWM.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    entry.cpp
    DwmDesktop.cpp
    WindowList.cpp
    DwmVisual.cpp
    MilResource.cpp)

add_library(uDWM MODULE
    ${SOURCE}
    uDWM.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uDWM.def
	${CMAKE_CURRENT_BINARY_DIR}/uDWM_stubs.c)

if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(uDWM PRIVATE /wd5043 /wd5033)
endif()

target_link_libraries(uDWM cppstl)

set_target_cpp_properties(uDWM WITH_EXCEPTIONS WITH_RTTI)
set_module_type(uDWM win32dll)
add_importlibs(uDWM milcore dwmredir msvcrt advapi32 gdi32 kernel32 ntdll user32)
add_dependencies(uDWM psdk)
add_cd_file(TARGET uDWM DESTINATION reactos/system32 FOR all)

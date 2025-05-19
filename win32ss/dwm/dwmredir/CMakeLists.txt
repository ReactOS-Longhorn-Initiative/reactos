
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(-DD3D9EXTENSIONS) #
include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

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

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_DWMREDIR_)
spec2def(dwmredir.dll dwmredir.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    entry.cpp
    stubs.cpp)

add_library(dwmredir MODULE
    ${SOURCE}
    dwmredir.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dwmredir.def
	${CMAKE_CURRENT_BINARY_DIR}/dwmredir_stubs.c)
if(MSVC)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/milcore_cpp)
# disable exception specification does not match previous declaration
target_compile_options(dwmredir PRIVATE /wd5043 /wd5033)
endif()

target_link_libraries(dwmredir cppstl)
set_target_cpp_properties(dwmredir WITH_EXCEPTIONS WITH_RTTI)
set_module_type(dwmredir win32dll)
add_importlibs(dwmredir msvcrt advapi32 kernel32 ntdll user32)
add_dependencies(dwmredir psdk)
add_cd_file(TARGET dwmredir DESTINATION reactos/system32 FOR all)

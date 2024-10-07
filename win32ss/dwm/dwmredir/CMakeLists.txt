
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_dwmredir_)
spec2def(dwmredir.dll dwmredir.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    entry.c
    stubs.c)

add_library(dwmredir MODULE
    ${SOURCE}
    dwmredir.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dwmredir.def
	${CMAKE_CURRENT_BINARY_DIR}/dwmredir_stubs.c)

set_module_type(dwmredir win32dll)
add_importlibs(dwmredir msvcrt kernel32 ntdll user32)
add_dependencies(dwmredir psdk)
#add_cd_file(TARGET dwmredir DESTINATION reactos/system32 FOR all)
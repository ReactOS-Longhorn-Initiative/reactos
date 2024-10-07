
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_uDWM_)
spec2def(uDWM.dll uDWM.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    entry.c
    CppEntry.cpp
    classes/CWindowList.cpp
    CDesktopManager.cpp
    stubs.c)

add_library(uDWM MODULE
    ${SOURCE}
    uDWM.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uDWM.def
	${CMAKE_CURRENT_BINARY_DIR}/uDWM_stubs.c)

set_module_type(uDWM win32dll)
add_importlibs(uDWM milcore dwmredir msvcrt kernel32 ntdll user32)
add_dependencies(uDWM psdk)
#add_cd_file(TARGET uDWM DESTINATION reactos/system32 FOR all)
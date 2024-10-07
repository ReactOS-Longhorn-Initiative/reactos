
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/evtlib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl
    ${CMAKE_CURRENT_BINARY_DIR})

list(APPEND SOURCE
    CDwmAppHost.cpp
    dwm.cpp
    entry.c
    dwm.h)

add_executable(dwm ${SOURCE} dwm.rc)
add_pch(dwm dwm.h SOURCE)

if(NOT MSVC)
    target_link_libraries(dwm ${PSEH_LIB})
endif()

set_module_type(dwm win32gui UNICODE)
target_link_libraries(dwm evtlib)
add_importlibs(dwm milcore user32 gdi32 advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET dwm DESTINATION reactos/system32 FOR all)

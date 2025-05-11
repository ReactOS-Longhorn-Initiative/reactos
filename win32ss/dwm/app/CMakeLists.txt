
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/evtlib
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl
    ${CMAKE_CURRENT_BINARY_DIR})

list(APPEND SOURCE
DWRUserFace.cpp
    dwr.cpp
    entry.c
    dwr.h)

add_executable(uxss ${SOURCE} dwm.rc)
add_pch(uxss dwr.h SOURCE)

if(NOT MSVC)
    target_link_libraries(uxss ${PSEH_LIB})
endif()

set_module_type(uxss win32gui UNICODE)
target_link_libraries(uxss evtlib)
add_importlibs(uxss user32 gdi32 advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET uxss DESTINATION reactos/system32 FOR all)

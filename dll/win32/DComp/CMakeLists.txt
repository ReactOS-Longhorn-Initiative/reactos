include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
add_definitions(-D__WINESRC__)
spec2def(DComp.dll DComp.spec)

list(APPEND SOURCE
    device.c
    ${CMAKE_CURRENT_BINARY_DIR}/DComp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/DComp.def)

add_library(DComp MODULE ${SOURCE})
set_module_type(DComp win32dll)
target_link_libraries(DComp wine)
add_importlibs(DComp user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET DComp DESTINATION reactos/system32 FOR all)
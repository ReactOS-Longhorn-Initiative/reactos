
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mf.dll mf.spec ADD_IMPORTLIBS)

list(APPEND SOURCE
    mf.c
    ${CMAKE_CURRENT_BINARY_DIR}/mf_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mf.def)

add_library(mf MODULE ${SOURCE})
set_module_type(mf win32dll)
target_link_libraries(mf wine)
add_importlibs(mf msvcrt kernel32 ntdll)
add_cd_file(TARGET mf DESTINATION reactos/system32 FOR all)


add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mfplat.dll mfplat.spec)

list(APPEND SOURCE
    mfplat.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfplat_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfplat.def)

add_library(mfplat MODULE ${SOURCE})
set_module_type(mfplat win32dll)
target_link_libraries(mfplat wine)
add_importlibs(mfplat msvcrt kernel32 ntdll)
add_cd_file(TARGET mfplat DESTINATION reactos/system32 FOR all)

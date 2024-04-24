
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(mscorwks.dll mscorwks.spec)

add_library(mscorwks MODULE
    ${CMAKE_CURRENT_BINARY_DIR}/mscorwks_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mscorwks.def)

set_module_type(mscorwks win32dll)
target_link_libraries(mscorwks uuid wine)
add_importlibs(mscorwks msvcrt kernel32 ntdll)
add_cd_file(TARGET mscorwks DESTINATION reactos/system32 FOR all)

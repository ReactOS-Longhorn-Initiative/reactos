
add_definitions(-D__WINESRC__)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(dcomp.dll dcomp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    device.c
    ${CMAKE_CURRENT_BINARY_DIR}/dcomp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dcomp.def)

add_library(dcomp SHARED ${SOURCE})
set_module_type(dcomp win32dll ENTRYPOINT 0)
target_link_libraries(dcomp wine wine_dll_canunload)
add_importlibs(dcomp shell32 shlwapi oleaut32 ole32 combase user32 user32_vista gdi32 ucrtbase advapi32 advapi32_vista kernel32 ntdll )
add_cd_file(TARGET dcomp DESTINATION reactos/system32 FOR all)


spec2def(rtworkq.dll rtworkq.spec ADD_IMPORTLIB)

remove_definitions(-D_WIN32_WINNT=0x502 -DNTDDI_VERSION)

add_definitions(-D_WIN32_WINNT=0x0A00)
add_definitions(-DNTDDI_VERSION=0x0A000005)

list(APPEND SOURCE
    queue.c
    ${CMAKE_CURRENT_BINARY_DIR}/rtworkq_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/rtworkq.def)

add_library(rtworkq MODULE ${SOURCE})
set_module_type(rtworkq win32dll UNICODE)
target_link_libraries(rtworkq wine)
add_importlibs(rtworkq ole32 msvcrt ntdll)
add_cd_file(TARGET rtworkq DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(rtworkq) # CORE-5743: No ARRAY_SIZE and CONST_VTABLE


spec2def(bcryptprimitives.dll bcryptprimitives.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/bcryptprimitives_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/bcryptprimitives.def
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c)

add_library(bcryptprimitives MODULE ${SOURCE})
set_module_type(bcryptprimitives win32dll UNICODE ENTRYPOINT 0)
add_importlibs(bcryptprimitives user32 advapi32 kernel32 ntdll)
target_link_libraries(bcryptprimitives wine)
set_wine_module(bcryptprimitives)

add_cd_file(TARGET bcryptprimitives DESTINATION reactos/system32 FOR all)

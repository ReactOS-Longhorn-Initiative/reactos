
spec2def(tbs.dll tbs.spec)

list(APPEND SOURCE
    tbs.c
    ${CMAKE_CURRENT_BINARY_DIR}/tbs_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/tbs.def)

add_library(tbs MODULE ${SOURCE})
set_module_type(tbs win32dll UNICODE ENTRYPOINT 0)
add_importlibs(tbs user32 kernel32 ntdll)
target_link_libraries(tbs wine)
set_wine_module(tbs)

add_cd_file(TARGET tbs DESTINATION reactos/system32 FOR all)

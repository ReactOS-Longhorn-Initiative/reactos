
add_definitions(-D_QAWAVE_)

spec2def(bcp47langs.dll bcp47langs.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c)

add_library(bcp47langs MODULE
    ${SOURCE}
    bcp47langs.rc
    ${CMAKE_CURRENT_BINARY_DIR}/bcp47langs_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/bcp47langs.def)

target_link_libraries(bcp47langs)
set_module_type(bcp47langs win32dll ENTRYPOINT DllMain)
target_link_libraries(bcp47langs uuid wine)
add_importlibs(bcp47langs advapi32 msvcrt kernel32 ntdll)
add_dependencies(bcp47langs wine psdk)
add_cd_file(TARGET bcp47langs DESTINATION reactos/system32 FOR all)

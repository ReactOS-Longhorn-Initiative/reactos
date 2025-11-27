
add_definitions(-D_QAWAVE_)

spec2def(profapi.dll profapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c)

add_library(profapi MODULE
    ${SOURCE}
    profapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/profapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/profapi.def)

target_link_libraries(profapi)
set_module_type(profapi win32dll ENTRYPOINT DllMain)
target_link_libraries(profapi uuid wine)
add_importlibs(profapi advapi32 msvcrt kernel32 ntdll)
add_dependencies(profapi wine psdk)
add_cd_file(TARGET profapi DESTINATION reactos/system32 FOR all)
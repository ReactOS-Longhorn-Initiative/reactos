
add_definitions(-D_QAWAVE_)

spec2def(coremessaging.dll coremessaging.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c)

add_library(coremessaging MODULE
    ${SOURCE}
    coremessaging.rc
    ${CMAKE_CURRENT_BINARY_DIR}/coremessaging_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/coremessaging.def)

target_link_libraries(coremessaging)
set_module_type(coremessaging win32dll ENTRYPOINT DllMain)
target_link_libraries(coremessaging uuid wine)
add_importlibs(coremessaging advapi32 msvcrt kernel32 ntdll)
add_dependencies(coremessaging wine psdk)
add_cd_file(TARGET coremessaging DESTINATION reactos/system32 FOR all)
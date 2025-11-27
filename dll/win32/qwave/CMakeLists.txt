
add_definitions(-D_QAWAVE_)

spec2def(qwave.dll qwave.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c)

add_library(qwave MODULE
    ${SOURCE}
    qwave.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qwave_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/qwave.def)

target_link_libraries(qwave)
set_module_type(qwave win32dll ENTRYPOINT DllMain)
target_link_libraries(qwave uuid wine)
add_importlibs(qwave advapi32 msvcrt kernel32 ntdll)
add_dependencies(qwave wine psdk)
add_cd_file(TARGET qwave DESTINATION reactos/system32 FOR all)

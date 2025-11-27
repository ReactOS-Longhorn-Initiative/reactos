
add_definitions(-D_QAWAVE_)

spec2def(rstrtmgr.dll rstrtmgr.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c)

add_library(rstrtmgr MODULE
    ${SOURCE}
    rstrtmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rstrtmgr_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/rstrtmgr.def)

target_link_libraries(rstrtmgr)
set_module_type(rstrtmgr win32dll ENTRYPOINT DllMain)
target_link_libraries(rstrtmgr uuid wine)
add_importlibs(rstrtmgr advapi32 msvcrt kernel32 ntdll)
add_dependencies(rstrtmgr wine psdk)
add_cd_file(TARGET rstrtmgr DESTINATION reactos/system32 FOR all)
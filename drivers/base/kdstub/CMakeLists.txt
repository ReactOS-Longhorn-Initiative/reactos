spec2def(kdstub.dll kdstub.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    kdstub.c
    win8_exports.c)

add_library(kdstub MODULE
    ${SOURCE}
    kdstub.rc
    ${CMAKE_CURRENT_BINARY_DIR}/kdstub.def)

set_module_type(kdstub kerneldll ENTRYPOINT 0)
set_subsystem(kdstub native)
add_cd_file(TARGET kdstub DESTINATION reactos/system32 NO_CAB FOR all)
target_link_libraries(kdstub cportlib blcmlib blrtl libcntpr)
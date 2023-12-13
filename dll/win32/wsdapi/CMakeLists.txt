
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

add_definitions(-D_WSDAPI_)
spec2def(WSDApi.dll WSDApi.spec)

list(APPEND SOURCE
    DllMain.c)

add_library(WSDApi MODULE
    ${SOURCE}
    WSDApi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/WSDApi.def
    ${CMAKE_CURRENT_BINARY_DIR}/WSDApi_stubs.c)

set_module_type(WSDApi win32dll ENTRYPOINT DllMain)
add_importlibs(WSDApi kernel32 ntdll)
add_dependencies(WSDApi psdk)
add_cd_file(TARGET WSDApi DESTINATION reactos/system32 FOR all)


spec2def(ndfapi.dll ndfapi.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/ndfapi_stubs.c)

add_library(ndfapi MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/ndfapi.def)

set_module_type(ndfapi win32dll)
add_importlibs(ndfapi msvcrt kernel32 ntdll)
add_cd_file(TARGET ndfapi DESTINATION reactos/system32 FOR all)

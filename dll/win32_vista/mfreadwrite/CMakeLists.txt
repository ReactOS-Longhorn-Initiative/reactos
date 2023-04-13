
spec2def(mfreadwrite.dll mfreadwrite.spec)

list(APPEND SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/mfreadwrite_stubs.c)

add_library(mfreadwrite MODULE
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/mfreadwrite.def)

set_module_type(mfreadwrite win32dll)
add_importlibs(mfreadwrite msvcrt kernel32 ntdll)
add_cd_file(TARGET mfreadwrite DESTINATION reactos/system32 FOR all)

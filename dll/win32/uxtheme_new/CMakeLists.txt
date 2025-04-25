
spec2def(uxtheme.dll uxtheme.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.cpp
    exports.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme_stubs.c)

add_library(uxtheme MODULE
    ${SOURCE}
    uxtheme.rc
    ${CMAKE_CURRENT_BINARY_DIR}/uxtheme.def)

set_module_type(uxtheme win32dll)
add_delay_importlibs(uxtheme msimg32)
add_importlibs(uxtheme user32 advapi32 gdiplus gdi32 shlwapi msvcrt kernel32 ntdll)
add_pch(uxtheme uxthemep.h SOURCE)
add_cd_file(TARGET uxtheme DESTINATION reactos/system32 FOR all)

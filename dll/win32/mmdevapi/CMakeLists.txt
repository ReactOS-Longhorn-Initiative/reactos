
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINE_REGISTER_DLL)

spec2def(mmdevapi.dll mmdevapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    DeviceEnumerator.cpp
    factory.cpp
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/mmdevapi_stubs.c)

add_library(mmdevapi MODULE
    ${SOURCE}
    guid.c
    mmdevapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mmdevapi.def)

set_module_type(mmdevapi win32dll)
target_link_libraries(mmdevapi uuid wine cppstl)
add_importlibs(mmdevapi ole32 oleaut32 user32 advapi32 kernel32 msvcrt ntdll)
add_pch(mmdevapi precomp.h SOURCE)
add_dependencies(mmdevapi psdk)
add_cd_file(TARGET mmdevapi DESTINATION reactos/system32 FOR all)
set_wine_module(mmdevapi)

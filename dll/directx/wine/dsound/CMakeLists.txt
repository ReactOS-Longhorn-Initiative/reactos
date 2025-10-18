

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

add_definitions(
    -D_WINE
    -D_USE_MATH_DEFINES)

spec2def(dsound.dll dsound.spec ADD_IMPORTLIB)

add_library(dsound MODULE
    buffer.c
    capture.c
    dsound.c
    dsound_convert.c
    dsound_main.c
    duplex.c
    mixer.c
    primary.c
    propset.c
    sound3d.c
    guid.c
    version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dsound.def)

set_module_type(dsound win32dll)
target_link_libraries(dsound dxguid uuid dxguid wine wine_dll_register oldnames)
add_importlibs(dsound winmm advapi32 msvcrt ole32 combase propsys kernelbase ntdll)
add_cd_file(TARGET dsound DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(dsound) # CORE-5743: No CONST_VTABLE

if (MSVC)
    target_compile_options(dsound PRIVATE /wd4305)
endif()
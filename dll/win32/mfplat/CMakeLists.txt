
spec2def(mfplat.dll mfplat.spec ADD_IMPORTLIB)

remove_definitions(-D_WIN32_WINNT=0x502 -DNTDDI_VERSION)

add_definitions(-D_WIN32_WINNT=0x0A00)
add_definitions(-DNTDDI_VERSION=0x0A000005)

add_definitions(
    # Workarounds
    -DI64_MIN=_I64_MIN
    -DI64_MAX=_I64_MAX
    -DUI64_MAX=_UI64_MAX
    -D__DO_ALIGN_DEFINES
)

list(APPEND SOURCE
    mediatype.c
    main.c
    buffer.c
    sample.c
    queue.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfplat_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfplat.def)

add_library(mfplat MODULE ${SOURCE})
set_module_type(mfplat win32dll UNICODE)
target_link_libraries(mfplat mfuuid dmoguids wine wine_dll_register oldnames)
add_delay_importlibs(mfplat bcrypt)
add_importlibs(mfplat advapi32 ole32 propsys rtworkq msvcrt kernelbase ntdll)
add_cd_file(TARGET mfplat DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mfplat) # CORE-5743: No ARRAY_SIZE and CONST_VTABLE

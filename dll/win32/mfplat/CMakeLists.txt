
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(mfplat.dll mfplat.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    buffer.c
    main.c
    mediatype.c
    queue.c
    sample.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfplat_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfplat.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(mfplat MODULE ${SOURCE})
set_module_type(mfplat win32dll)
target_link_libraries(mfplat wine uuid dmoguids mfuuid dxguid strmbase strmiids oldnames wine_dll_register)
add_importlibs(mfplat rtworkq bcrypt shlwapi propsys advapi32 user32 ole32 oleaut32 d3d9 msvcrt kernel32 kernel32_vista kernelbase_ros ntdll)
add_cd_file(TARGET mfplat DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mfplat) # CORE-5743: ???
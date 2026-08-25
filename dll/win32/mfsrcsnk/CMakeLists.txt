
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(mfsrcsnk.dll mfsrcsnk.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    factory.c
    media_sink.c
    media_source.c
    wave.c
    mfsrcsnk.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mfsrcsnk_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfsrcsnk.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(mfsrcsnk MODULE ${SOURCE})
set_module_type(mfsrcsnk win32dll)
add_idl_reg_scripts(mfsrcsnk registry mfsrcsnk.idl)
target_link_libraries(mfsrcsnk wine wine_dll_canunload uuid dmoguids mfuuid dxguid strmbase strmiids oldnames wine_dll_register)
add_importlibs(mfsrcsnk mf mfplat winedmo bcrypt shlwapi propsys advapi32 user32 ole32 oleaut32 d3d9 msvcrt kernel32 kernel32_vista kernelbase_ros ntdll)
add_cd_file(TARGET mfsrcsnk DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mfsrcsnk) # CORE-5743: ???
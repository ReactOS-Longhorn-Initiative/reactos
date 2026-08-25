
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(mfreadwrite.dll mfreadwrite.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    reader.c
    writer.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfreadwrite_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfreadwrite.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(mfreadwrite MODULE ${SOURCE})
set_module_type(mfreadwrite win32dll)
add_idl_reg_scripts(mfreadwrite registry mf_classes.idl)
target_link_libraries(mfreadwrite wine wine_dll_canunload uuid dmoguids mfuuid dxguid strmbase strmiids oldnames wine_dll_register)
add_importlibs(mfreadwrite mf mfplat bcrypt shlwapi propsys advapi32 user32 ole32 oleaut32 d3d9 msvcrt kernel32 kernel32_vista kernelbase_ros ntdll)
add_cd_file(TARGET mfreadwrite DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mfreadwrite) # CORE-5743: ???
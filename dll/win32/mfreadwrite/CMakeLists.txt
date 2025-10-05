
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
add_idl_headers(mfreadwrite_idlheader mf_classes.idl)
set_module_type(mfreadwrite win32dll)
add_dependencies(mfreadwrite mfreadwrite_idlheader)
target_link_libraries(mfreadwrite pathcch wine uuid dmoguids mfuuid dxguid strmbase strmiids oldnames wine_dll_register)
add_importlibs(mfreadwrite mf mfplat bcrypt shlwapi propsys advapi32 user32 ole32 oleaut32 d3d9 msvcrt kernelbase ntdll)
add_cd_file(TARGET mfreadwrite DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mfreadwrite) # CORE-5743: ???
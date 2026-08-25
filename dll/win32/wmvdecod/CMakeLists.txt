
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DUSE_NEW_WINE_REGISTER_RESOURCES)
spec2def(wmvdecod.dll wmvdecod.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    video_decoder.c
    wmvdecod.c
    ${CMAKE_CURRENT_BINARY_DIR}/wmvdecod_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wmvdecod.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(wmvdecod MODULE ${SOURCE})
set_module_type(wmvdecod win32dll ENTRYPOINT 0)
add_idl_reg_scripts(wmvdecod registry wmvdecod.idl)
add_importlibs(wmvdecod msdmo ole32 oleaut32 mf mfplat user32 ucrtbase kernel32 kernel32_vista ntdll)
target_link_libraries(wmvdecod mfuuid dmoguids dxguid uuid strmiids wmcodecdspuuid wine_dll_canunload)
add_cd_file(TARGET wmvdecod DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(wmvdecod) # CORE-5743: ???

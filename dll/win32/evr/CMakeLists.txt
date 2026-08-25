

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(evr.dll evr.spec ADD_IMPORTLIB)

add_idl_headers(evr_idlheader evr_classes.idl)

list(APPEND SOURCE
    evr.c
    main.c
    mixer.c
    presenter.c
    sample.c
    evr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/evr_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/evr.def)

add_library(evr MODULE ${SOURCE})
set_module_type(evr win32dll)
add_idl_reg_scripts(evr registry evr_classes.idl)
add_dependencies(evr evr_idlheader)
target_link_libraries(evr wine uuid mfuuid dxguid strmbase strmiids wine_dll_register wine_dll_canunload)
add_importlibs(evr mfplat dxva2 user32 ole32 oleaut32 d3d9 msvcrt kernel32 kernel32_vista ntdll)
add_cd_file(TARGET evr DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(evr) # CORE-5743: ???
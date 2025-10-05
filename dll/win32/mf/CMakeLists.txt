
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(mf.dll mf.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    clock.c
    copier.c
    evr.c
    main.c
    quality.c
    sac.c
    samplegrabber.c
    sar.c
    scheme_handler.c
    session.c
    topology_loader.c
    topology.c
    mf.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mf_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mf.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(mf MODULE ${SOURCE})
set_module_type(mf win32dll)
target_link_libraries(mf pathcch wine uuid dmoguids mfuuid dxguid strmbase strmiids oldnames wine_dll_register)
add_importlibs(mf mfplat evr propsys urlmon advapi32 user32 ole32 oleaut32 d3d9 msvcrt kernelbase ntdll)
add_cd_file(TARGET mf DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mf) # CORE-5743: ???
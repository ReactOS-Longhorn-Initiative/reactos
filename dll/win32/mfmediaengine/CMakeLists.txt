
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(mfmediaengine.dll mfmediaengine.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    video_frame_sink.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfmediaengine_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfmediaengine.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(mfmediaengine MODULE ${SOURCE})
set_module_type(mfmediaengine win32dll)
add_idl_reg_scripts(mfmediaengine registry mediaengine_classes.idl)
add_importlibs(mfmediaengine mfplat mf ole32 oleaut32 d3d11 user32 msvcrt kernel32 ntdll)
target_link_libraries(mfmediaengine wine_dll_register wine_dll_canunload uuid mfuuid dmoguids dxguid strmiids oldnames)
add_cd_file(TARGET mfmediaengine DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mfmediaengine) # CORE-5743: ???
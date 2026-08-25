
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/compat)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/) # TODO REMOVE
include_directories(${REACTOS_SOURCE_DIR}/dll/win32/wmvdecod)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DUSE_NEW_WINE_REGISTER_RESOURCES)
spec2def(msmpeg2vdec.dll msmpeg2vdec.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    msmpeg2vdec.c
    ../winedmo/reactos.c
    ../wmvdecod/video_decoder.c
    ${CMAKE_CURRENT_BINARY_DIR}/msmpeg2vdec_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msmpeg2vdec.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(msmpeg2vdec MODULE ${SOURCE})
set_module_type(msmpeg2vdec win32dll ENTRYPOINT 0)
add_idl_reg_scripts(msmpeg2vdec registry msmpeg2vdec.idl)
add_importlibs(msmpeg2vdec msdmo ole32 oleaut32 mf mfplat user32 ucrtbase kernel32 kernel32_vista ntdll)
target_link_libraries(msmpeg2vdec mfuuid dmoguids dxguid uuid strmiids wmcodecdspuuid wine_dll_canunload)
add_cd_file(TARGET msmpeg2vdec DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(msmpeg2vdec) # CORE-5743: ???

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/compat)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DUSE_NEW_WINE_REGISTER_RESOURCES)
spec2def(msvproc.dll msvproc.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    msvproc.c
    video_processor.c
    ../winedmo/reactos.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvproc_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvproc.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(msvproc MODULE ${SOURCE})
set_module_type(msvproc win32dll ENTRYPOINT 0)
add_idl_reg_scripts(msvproc registry msvproc.idl)
add_importlibs(msvproc ole32 oleaut32 mf mfplat user32 ucrtbase kernel32 kernel32_vista ntdll)
target_link_libraries(msvproc mfuuid dxguid uuid strmiids wmcodecdspuuid wine_dll_canunload)
add_cd_file(TARGET msvproc DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(msvproc) # CORE-5743: ???
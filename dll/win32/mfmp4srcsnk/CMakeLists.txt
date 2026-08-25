
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/compat)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/)
include_directories(${REACTOS_SOURCE_DIR}/dll/win32/mfsrcsnk)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(mfmp4srcsnk.dll mfmp4srcsnk.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    mfmp4srcsnk.c
    mfmp4srcsnk.rc
    ${REACTOS_SOURCE_DIR}/dll/win32/mfsrcsnk/media_sink.c
    ${REACTOS_SOURCE_DIR}/dll/win32/mfsrcsnk/media_source.c
    ${REACTOS_SOURCE_DIR}/dll/win32/mfsrcsnk/wave.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfmp4srcsnk_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mfmp4srcsnk.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(mfmp4srcsnk MODULE ${SOURCE})
set_module_type(mfmp4srcsnk win32dll)
add_idl_reg_scripts(mfmp4srcsnk registry mfmp4srcsnk.idl)
target_link_libraries(mfmp4srcsnk wine wine_dll_canunload uuid dmoguids mfuuid dxguid strmbase strmiids oldnames wine_dll_register)
add_importlibs(mfmp4srcsnk mf mfplat winedmo bcrypt shlwapi propsys advapi32 user32 ole32 oleaut32 d3d9 msvcrt kernel32 kernel32_vista kernelbase_ros ntdll)
add_cd_file(TARGET mfmp4srcsnk DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(mfmp4srcsnk) # CORE-5743: ???
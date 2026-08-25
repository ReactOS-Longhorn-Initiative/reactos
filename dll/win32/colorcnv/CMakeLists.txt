
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/compat)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DUSE_NEW_WINE_REGISTER_RESOURCES)
spec2def(colorcnv.dll colorcnv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    color_converter.c
    colorcnv.c
    ../winedmo/reactos.c
    ${CMAKE_CURRENT_BINARY_DIR}/colorcnv_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/colorcnv.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(colorcnv MODULE ${SOURCE})
set_module_type(colorcnv win32dll ENTRYPOINT 0)
add_idl_reg_scripts(colorcnv registry colorcnv.idl)
add_importlibs(colorcnv msdmo ole32 oleaut32 mf mfplat user32 ucrtbase kernel32 kernel32_vista ntdll)
target_link_libraries(colorcnv mfuuid dmoguids dxguid uuid strmiids wmcodecdspuuid wine_dll_canunload)
add_cd_file(TARGET colorcnv DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(colorcnv) # CORE-5743: ???
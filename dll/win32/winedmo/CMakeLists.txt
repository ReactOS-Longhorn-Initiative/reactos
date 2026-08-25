
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(winedmo.dll winedmo.spec ADD_IMPORTLIB)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/compat)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/ffmpeg/)


list(APPEND SOURCE
    main.c
    reactos.c
    unix_demuxer.c
    unix_media_type.c
    unixlib.c
    ${CMAKE_CURRENT_BINARY_DIR}/winedmo_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winedmo.def)

remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
add_library(winedmo MODULE ${SOURCE})
set_module_type(winedmo win32dll)
target_link_libraries(winedmo wine uuid dmoguids mfuuid dxguid strmbase strmiids wine_dll_register)
add_importlibs(winedmo rtworkq bcrypt shlwapi propsys advapi32 user32 ole32 oleaut32 d3d9 ucrtbase msvcrt kernel32 kernel32_vista kernelbase_ros ntdll)
add_cd_file(TARGET winedmo DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(winedmo) # CORE-5743: ???

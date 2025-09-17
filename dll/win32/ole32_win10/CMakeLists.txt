
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(ole32_win10.dll ole32_win10.spec ADD_IMPORTLIB)
add_idl_headers(combase_idlheader dcom.idl)
add_rpc_files(client irpcss.idl)

list(APPEND SOURCE
    apartment.c
    combase.c
    errorinfo.c
    hglobalstream.c
    malloc.c
    marshal.c
    roapi.c
    rpc.c
    string.c
    stubmanager.c
    usrmarshal.c
    ${CMAKE_CURRENT_BINARY_DIR}/ole32_win10_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/irpcss_c.c
)

add_definitions(-Dwcsdup=_wcsdup -Dwcsicmp=_wcsicmp -D_ROAPI_)

add_library(ole32_win10 MODULE ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/ole32_win10.def)
set_module_type(ole32_win10 win32dll)
target_link_libraries(ole32_win10 wine uuid pseh)
add_importlibs(ole32_win10 advapi32 ole32 oleaut32 user32 gdi32 rpcrt4 msvcrt kernelbase ntdll)
add_dependencies(ole32_win10 combase_idlheader wineheaders rpcss)
add_cd_file(TARGET ole32_win10 DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(ole32_win10) # CORE-5743: ???

if(MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    # Disable warning C4312: 'type cast': conversion from 'unsigned int' to 'void *' of greater size
    target_compile_options(ole32_win10 PRIVATE /wd4090 /wd4312)
endif()


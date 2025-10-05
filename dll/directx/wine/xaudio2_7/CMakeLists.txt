
spec2def(xaudio2_7.dll xaudio2_7.spec)

list(APPEND SOURCE
    x3daudio.c
    xapo.c
    xapofx.c
    xaudio_allocator.c
    xaudio_dll.c
	faudio/src/F3DAudio.c
	faudio/src/FACT.c
	faudio/src/FACT3D.c
	faudio/src/FACT_internal.c
	faudio/src/FAPOBase.c
	faudio/src/FAPOFX.c
	faudio/src/FAPOFX_echo.c
	faudio/src/FAPOFX_eq.c
	faudio/src/FAPOFX_masteringlimiter.c
	faudio/src/FAPOFX_reverb.c
	faudio/src/FAudio.c
	faudio/src/FAudioFX_reverb.c
	faudio/src/FAudioFX_volumemeter.c
	faudio/src/FAudio_internal.c
	faudio/src/FAudio_internal_simd.c
	faudio/src/FAudio_operationset.c
	faudio/src/FAudio_platform_win32.c
	faudio/src/matrix_defaults.inl
    ${CMAKE_CURRENT_BINARY_DIR}/xaudio2_7.def)

add_idl_headers(xaudio2_7_idlheader xaudio_classes.idl)
add_library(xaudio2_7 MODULE ${SOURCE} version.rc)
add_definitions(-DXAUDIO2_VER=7 -DFAUDIO_WIN32_PLATFORM -DHAVE_WMADEC -DNDEBUG)
set_module_type(xaudio2_7 win32dll)
add_importlibs(xaudio2_7 propsys ole32 mfplat mfreadwrite msvcrt kernel32 ntdll)
target_link_libraries(xaudio2_7 wine wine_dll_register uuid mfuuid)
add_dependencies(xaudio2_7 xaudio2_7_idlheader)
include_directories(${REACTOS_SOURCE_DIR}/dll/directx/wine/xaudio2_7/faudio/include/)

if(MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    # Disable warning C4312: 'type cast': conversion from 'unsigned int' to 'void *' of greater size
    # Disable warning C4189: local variable intialized but not referenced
    target_compile_options(xaudio2_7 PRIVATE /wd4090 /wd4312 /wd4189)
endif()


add_cd_file(TARGET xaudio2_7 DESTINATION reactos/system32 FOR all)
set_wine_module(xaudio2_7)

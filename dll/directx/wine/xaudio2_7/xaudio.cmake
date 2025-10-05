
function(define_xaudio xaudio_name version)
    spec2def(${xaudio_name}.dll ${xaudio_name}.spec)

    list(APPEND SOURCE
        ../xaudio2_7/x3daudio.c
        ../xaudio2_7/xapo.c
        ../xaudio2_7/xapofx.c
        ../xaudio2_7/xaudio_allocator.c
        ../xaudio2_7/xaudio_dll.c
        ${CMAKE_CURRENT_BINARY_DIR}/${xaudio_name}.def)

    set(OLD_IDL_FLAGS ${IDL_FLAGS})
    set(IDL_FLAGS ${IDL_FLAGS} -DXAUDIO2_VER=${version})
    add_idl_headers(${xaudio_name}_idlheader ../xaudio2_7/xaudio_classes.idl)
    set(IDL_FLAGS ${OLD_IDL_FLAGS})

    add_library(${xaudio_name} MODULE ${SOURCE} ../xaudio2_7/version.rc)
    add_definitions(-DXAUDIO2_VER=${version})
    set_module_type(${xaudio_name} win32dll)
    add_importlibs(${xaudio_name} propsys ole32 mfplat mfreadwrite msvcrt kernel32 ntdll)
    target_link_libraries(${xaudio_name} wine wine_dll_register uuid mfuuid faudio)
    add_dependencies(${xaudio_name} ${xaudio_name}_idlheader)

    if(MSVC)
        # Disable warning C4090: 'function': different 'const' qualifiers
        # Disable warning C4312: 'type cast': conversion from 'unsigned int' to 'void *' of greater size
        # Disable warning C4189: local variable intialized but not referenced
        target_compile_options(${xaudio_name} PRIVATE /wd4090 /wd4312 /wd4189)
    endif()


    add_cd_file(TARGET ${xaudio_name} DESTINATION reactos/system32 FOR all)
    set_wine_module(${xaudio_name})
endfunction()

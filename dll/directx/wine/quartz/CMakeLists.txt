
remove_definitions(-D_WIN32_WINNT=0x502 -D_CRT_NON_CONFORMING_SWPRINTFS)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -DENTRY_PREFIX=QUARTZ_
    -DWINE_REGISTER_DLL
    -DPROXY_DELEGATION)

spec2def(quartz.dll quartz.spec)
add_rpcproxy_files(quartz_strmif.idl)

list(APPEND SOURCE
    acmwrapper.c
    avidec.c
    decoder.c
    dsoundrender.c
    filesource.c
    filtergraph.c
    filtermapper.c
    main.c
    memallocator.c
    memallocator.c
    parser.c
    passthrough.c
    systemclock.c
    videorenderer.c
    vmr7_presenter.c
    vmr7.c
    vmr9.c
    window.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/quartz_strmif_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/quartz_stubs.c)

add_library(quartz MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/quartz.def
    quartz.rc)

add_typelib(control_tlb.idl)

list(APPEND quartz_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/control_tlb_t.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/control_tlb_r.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/control_tlb.tlb)

set_source_files_properties(quartz.rc PROPERTIES OBJECT_DEPENDS "${quartz_rc_deps}")
set_module_type(quartz win32dll)
target_link_libraries(quartz strmbase strmiids uuid dxguid wine oldnames ${PSEH_LIB})
add_importlibs(quartz dsound ddraw winmm msacm32 msvfw32 ole32 oleaut32 rpcrt4 user32 gdi32 advapi32 advapi32_vista shlwapi msvcrt kernel32 ntdll)
add_dependencies(quartz dxsdk stdole2)
add_pch(quartz precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET quartz DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(quartz) # CORE-5743: No CONST_VTABLE

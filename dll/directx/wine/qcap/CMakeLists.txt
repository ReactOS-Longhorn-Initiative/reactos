
remove_definitions(-D_WIN32_WINNT=0x502 -D_CRT_NON_CONFORMING_SWPRINTFS)
add_definitions(-D_WIN32_WINNT=0x600)
spec2def(qcap.dll qcap.spec)

list(APPEND SOURCE
    audiorecord.c
    avico.c
    avimux.c
    capturegraph.c
    filewriter.c
    qcap_main.c
    smartteefilter.c
    v4l.c
    vfwcapture.c)

add_library(qcap MODULE ${SOURCE}
           ${CMAKE_CURRENT_BINARY_DIR}/qcap_stubs.c
           ${CMAKE_CURRENT_BINARY_DIR}/qcap.def
           version.rc)

set_module_type(qcap win32dll)
target_link_libraries(qcap strmbase strmiids uuid wine oldnames)
add_importlibs(qcap user32 winmm ole32 oleaut32 gdi32 advapi32 advapi32_vista msvcrt kernel32 ntdll)
add_delay_importlibs(qcap msvfw32)
add_cd_file(TARGET qcap DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(qcap) # CORE-5743: No CONST_VTABLE

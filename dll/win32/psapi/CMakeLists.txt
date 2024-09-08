
spec2def(psapi.dll psapi.spec ADD_IMPORTLIB)


add_library(win32_psapi
    psapi.c
)
target_link_libraries(win32_psapi ${PSEH_LIB})


list(APPEND SOURCE
    psapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/psapi.def)

add_library(psapi MODULE ${SOURCE})
set_module_type(psapi win32dll ENTRYPOINT DllMain 12)
target_link_libraries(psapi win32_psapi ${PSEH_LIB})
add_importlibs(psapi kernel32 ntdll)
add_cd_file(TARGET psapi DESTINATION reactos/system32 FOR all)

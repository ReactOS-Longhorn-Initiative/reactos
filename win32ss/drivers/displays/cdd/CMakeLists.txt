
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x601)

list(APPEND SOURCE
    anotherblt.cpp
    cdd.cpp
    entry.cpp)

add_library(cdd MODULE
    ${SOURCE}
    cdd.rc)

set_module_type(cdd kerneldll ENTRYPOINT DrvEnableDriver 12)
target_link_libraries(cdd libcntpr)
add_importlibs(cdd win32k ntoskrnl)
add_cd_file(TARGET cdd DESTINATION reactos/system32 FOR all)

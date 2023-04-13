
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_clbcatq_)
spec2def(clbcatq.dll clbcatq.spec)

list(APPEND SOURCE
    DllMain.c
    stubs.c)

add_library(clbcatq MODULE
    ${SOURCE}
    clbcatq.rc
    ${CMAKE_CURRENT_BINARY_DIR}/clbcatq.def
	${CMAKE_CURRENT_BINARY_DIR}/clbcatq_stubs.c)

set_module_type(clbcatq win32dll ENTRYPOINT DllMain)
add_importlibs(clbcatq kernel32 ntdll msvcrt user32)
add_dependencies(clbcatq psdk)
add_cd_file(TARGET clbcatq DESTINATION reactos/system32 FOR all)
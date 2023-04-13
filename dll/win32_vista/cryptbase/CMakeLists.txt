
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x601 -DWINVER=0x601)

include_directories(${REACTOS_SOURCE_DIR}/win32ss/include)

add_definitions(-D_cryptbase_)
spec2def(cryptbase.dll cryptbase.spec)

list(APPEND SOURCE
    DllMain.c
    stubs.c)

add_library(cryptbase MODULE
    ${SOURCE}
    cryptbase.rc
    ${CMAKE_CURRENT_BINARY_DIR}/cryptbase.def
	${CMAKE_CURRENT_BINARY_DIR}/cryptbase_stubs.c)

set_module_type(cryptbase win32dll ENTRYPOINT DllMain)
add_importlibs(cryptbase kernel32 ntdll msvcrt user32)
add_dependencies(cryptbase psdk)
add_cd_file(TARGET cryptbase DESTINATION reactos/system32 FOR all)

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600)

add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__
    -D_WINE
    -D_ncrypt_)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(ncrypt.dll ncrypt.spec ADD_IMPORTLIB)

list(APPEND SOURCE
stubs.c)
   # main.c)

add_library(ncrypt MODULE
    ${SOURCE}
    ncrypt.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ncrypt_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ncrypt.def)

if(MSVC)
    # Disable warning C4090: 'function': different 'const' qualifiers
    # Disable warning C4312: 'type cast': conversion from 'unsigned int' to 'void *' of greater size
    target_compile_options(ncrypt PRIVATE /wd4090 /wd4312)
endif()

set_module_type(ncrypt win32dll ENTRYPOINT 0)
target_link_libraries(ncrypt wine ${PSEH_LIB} oldnames)
add_delay_importlibs(ncrypt cryptnet)
add_importlibs(ncrypt bcrypt user32 advapi32_vista advapi32 msvcrt kernel32 ntdll)
#add_pch(ncrypt precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET ncrypt DESTINATION reactos/system32 FOR all)


add_definitions(-D_QAWAVE_)

spec2def(wevtapi.dll wevtapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c)

add_library(wevtapi MODULE
    ${SOURCE}
    wevtapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wevtapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wevtapi.def)

if(MSVC)
    # Disable warning C4312: 'type cast': conversion from 'unsigned int' to 'HANDLE' of greater size
    target_compile_options(wevtapi PRIVATE /wd4312)
endif()

target_link_libraries(wevtapi)
set_module_type(wevtapi win32dll ENTRYPOINT DllMain)
target_link_libraries(wevtapi uuid wine)
add_importlibs(wevtapi advapi32 msvcrt kernel32 ntdll)
add_dependencies(wevtapi wine psdk)
add_cd_file(TARGET wevtapi DESTINATION reactos/system32 FOR all)
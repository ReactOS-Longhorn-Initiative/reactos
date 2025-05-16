include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/LpcConnectLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/LpcCreateLib)
include_directories(${REACTOS_SOURCE_DIR}/win32ss/dwm/shared/include)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/rwm)

spec2def(uxsms.dll uxsms.spec ADD_IMPORTLIB)

add_library(uxsms MODULE
    dwmsession.cpp
    port.cpp    
    serv.cpp
    uxsms.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/uxsms_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/uxsms.def)

set_module_type(uxsms win32dll UNICODE)
target_link_libraries(uxsms LpcCreateLib LpcConnectLib wine)
add_importlibs(uxsms uxtheme wtsapi32 advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET uxsms DESTINATION reactos/system32 FOR all)

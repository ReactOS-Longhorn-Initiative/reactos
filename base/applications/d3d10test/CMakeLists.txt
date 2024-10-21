add_definitions(-D_USRDLL -D_FORCENAMELESSUNION)


add_executable(d3d10_hello_world
    d3d10hw.c
)

set_module_type(d3d10_hello_world win32gui)
add_dependencies(d3d10_hello_world psdk dxsdk)
add_dependencies(d3d10_hello_world wineheaders d3d_idl_headers)
add_importlibs(d3d10_hello_world d3d10core d3d10 dxgi advapi32 comctl32 user32 shell32 gdi32 msvcrt kernel32 ntdll)

target_link_libraries(d3d10_hello_world cppstl atl_classes dxguid)
set_target_properties(d3d10_hello_world PROPERTIES WITH_CXX_EXCEPTIONS TRUE)
add_cd_file(TARGET d3d10_hello_world DESTINATION reactos/system32 FOR all)

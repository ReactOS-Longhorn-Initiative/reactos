add_definitions(-D_USRDLL -D_FORCENAMELESSUNION)

add_executable(d3d10_hello_world
    d3d10hw.cpp
)

set_module_type(calc win32gui)
add_dependencies(d3d10_hello_world psdk dxsdk)
target_link_libraries(d3d10_hello_world cppstl atl_classes dxguid d3d10)
set_target_properties(d3d10_hello_world PROPERTIES WITH_CXX_EXCEPTIONS TRUE)

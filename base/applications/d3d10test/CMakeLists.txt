add_definitions(-D_USRDLL -D_FORCENAMELESSUNION)

add_library(d3d10_hello_world MODULE
    d3d10hw.cpp
)

set_module_type(d3d10_hello_world win32dll UNICODE)
add_dependencies(d3d10_hello_world psdk dxsdk)
target_link_libraries(d3d10_hello_world cppstl atl_classes dxguid d3d10)
set_target_properties(d3d10_hello_world PROPERTIES WITH_CXX_EXCEPTIONS TRUE)

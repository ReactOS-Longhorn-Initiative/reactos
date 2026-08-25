
add_library(dxguid dx8guid.c dx9guid.c dx10guid.c dxguid.c)
add_library(dx10guid dx10guid.c)

add_dependencies(dxguid psdk)
add_dependencies(dx10guid psdk d3d_idl_headers)


add_compile_definitions(-DWIN32)
add_library(hbaapi HBAAPILIB.c)
add_dependencies(hbaapi xdk)

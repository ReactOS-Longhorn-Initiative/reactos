
add_definitions(-D_NTSYSTEM_)

list(APPEND SOURCE
    scsiwmi.c)

add_library(scsiwmi ${SOURCE})
add_dependencies(scsiwmi xdk)

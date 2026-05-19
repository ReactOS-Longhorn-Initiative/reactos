
list(APPEND SOURCE
    dmoguids.c)

add_library(dmoguids ${SOURCE})
add_dependencies(dmoguids psdk)

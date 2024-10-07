include_directories(".")

list(APPEND SOURCE
    DllMain.c
    KB.h)

add_library(UtilLib
            ${SOURCE})

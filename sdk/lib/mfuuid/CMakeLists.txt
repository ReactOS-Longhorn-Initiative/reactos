
list(APPEND SOURCE
    mfuuid.c
    mfplay.c)

add_library(mfuuid ${SOURCE})
add_dependencies(mfuuid wineheaders psdk)
